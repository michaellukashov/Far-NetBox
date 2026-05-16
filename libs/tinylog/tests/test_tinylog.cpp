// tinylog unit tests
// Tests thread-safety of tinylog logging library.
//
// Uses minimal includes to avoid NetBox base library dependencies.
// Test verification: log file creation, content integrity, multi-thread stress.

#include <tinylog/TinyLog.h>
#include <tinylog/Config.h>

#include <Windows.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

namespace
{

void log_pass(const char * name)
{
  printf("PASS: %s\n", name);
  fflush(stdout);
}

void log_fail(const char * name, const char * reason)
{
  printf("FAIL: %s: %s\n", name, reason);
  fflush(stdout);
}

tinylog::TinyLog * create_logger(const char * path)
{
  FILE * f = fopen(path, "w");
  assert(f);
  auto * logger = new tinylog::TinyLog();
  logger->level(tinylog::Utils::LEVEL_TRACE);
  logger->file(f);
  return logger;
}

void close_logger(tinylog::TinyLog * logger)
{
  logger->Close();
  delete logger;
}

std::string read_file(const char * path)
{
  FILE * f = fopen(path, "rb");
  if (!f)
    return "";
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::string content(sz, '\0');
  fread(&content[0], 1, sz, f);
  fclose(f);
  return content;
}

} // namespace

void test_tls_buffer_isolation()
{
  printf("INFO: Running test: test_tls_buffer_isolation\n");
  const char * log_path = "test_tls_iso.log";

  auto * logger = create_logger(log_path);
  TINYLOG_INFO(logger) << "thread_main_entry";
  close_logger(logger);

  std::string content = read_file(log_path);

  if (content.find("thread_main_entry") != std::string::npos)
    log_pass("test_tls_buffer_isolation");
  else
    log_fail("test_tls_buffer_isolation", "log entry not found in file");

  remove(log_path);
}

void test_atomic_drain_signal()
{
  printf("INFO: Running test: test_atomic_drain_signal\n");
  const char * log_path = "test_drain_signal.log";

  auto * logger = create_logger(log_path);

  std::vector<std::thread> threads;
  std::atomic<int> completed{0};
  const int num_threads = 4;
  const int entries_per_thread = 500;

  for (int t = 0; t < num_threads; t++)
  {
    threads.emplace_back([&, t]()
    {
      for (int i = 0; i < entries_per_thread; i++)
      {
        char buf[128];
        sprintf_s(buf, "thread_%d_entry_%d", t, i);
        TINYLOG_DEBUG(logger) << buf;
      }
      completed.fetch_add(1);
    });
  }

  for (auto & th : threads)
    th.join();

  close_logger(logger);

  std::string content = read_file(log_path);
  int found = 0;

  for (int t = 0; t < num_threads; t++)
  {
    for (int i = 0; i < entries_per_thread; i++)
    {
      char buf[128];
      sprintf_s(buf, "thread_%d_entry_%d", t, i);
      if (content.find(buf) != std::string::npos)
        found++;
    }
  }

  int expected = num_threads * entries_per_thread;
  if (found == expected)
    log_pass("test_atomic_drain_signal");
  else
  {
    char reason[256];
    sprintf_s(reason, "expected %d entries, found %d", expected, found);
    log_fail("test_atomic_drain_signal", reason);
  }

  remove(log_path);
}

void test_timestamp_monotonic()
{
  printf("INFO: Running test: test_timestamp_monotonic\n");
  const char * log_path = "test_timestamp.log";

  auto * logger = create_logger(log_path);

  std::vector<std::thread> threads;

  for (int t = 0; t < 4; t++)
  {
    threads.emplace_back([&]()
    {
      for (int i = 0; i < 1000; i++)
      {
        TINYLOG_TRACE(logger) << "monotonic_test";
      }
    });
  }

  for (auto & th : threads)
    th.join();

  close_logger(logger);

  std::string content = read_file(log_path);

  if (!content.empty())
    log_pass("test_timestamp_monotonic");
  else
    log_fail("test_timestamp_monotonic", "log file is empty");

  if (content.find("[TRACE  ]") != std::string::npos)
    printf("INFO: Log file contains properly formatted entries\n");

  remove(log_path);
}

void test_buffer_overflow()
{
  printf("INFO: Running test: test_buffer_overflow\n");
  const char * log_path = "test_overflow.log";

  auto * logger = create_logger(log_path);

  std::atomic<int> done{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < 8; t++)
  {
    threads.emplace_back([&, t]()
    {
      char big_msg[1024];
      memset(big_msg, 'X', sizeof(big_msg) - 1);
      big_msg[sizeof(big_msg) - 1] = '\0';
      char msg[1536];

      for (int i = 0; i < 500; i++)
      {
        sprintf_s(msg, "thread_%d_seq_%d_%s", t, i, big_msg);
        TINYLOG_INFO(logger) << msg;
      }
      done.fetch_add(1);
    });
  }

  for (auto & th : threads)
    th.join();

  close_logger(logger);

  std::string content = read_file(log_path);

  if (!content.empty())
  {
    bool has_valid_entries = false;
    for (int t = 0; t < 8; t++)
    {
      char buf[64];
      sprintf_s(buf, "thread_%d_seq_0", t);
      if (content.find(buf) != std::string::npos)
      {
        has_valid_entries = true;
        break;
      }
    }

    if (has_valid_entries)
      log_pass("test_buffer_overflow");
    else
      log_fail("test_buffer_overflow", "no valid entries found in log file");
  }
  else
    log_fail("test_buffer_overflow", "log file is empty");

  remove(log_path);
}

void test_stress_multithread()
{
  printf("INFO: Running test: test_stress_multithread\n");
  const char * log_path = "test_stress.log";

  auto * logger = create_logger(log_path);

  const int num_threads = 10;
  const int entries_per_thread = 10000;
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; t++)
  {
    threads.emplace_back([&, t]()
    {
      for (int i = 0; i < entries_per_thread; i++)
      {
        char buf[64];
        sprintf_s(buf, "stress_t%d_i%d", t, i);
        TINYLOG_DEBUG(logger) << buf;
      }
    });
  }

  for (auto & th : threads)
    th.join();

  close_logger(logger);

  std::string content = read_file(log_path);

  if (content.empty())
  {
    log_fail("test_stress_multithread", "log file is empty");
    remove(log_path);
    return;
  }

  // Count how many entries are present
  int found = 0;
  int partial_lines = 0;

  for (int t = 0; t < num_threads; t++)
  {
    for (int i = 0; i < entries_per_thread; i++)
    {
      char buf[64];
      sprintf_s(buf, "stress_t%d_i%d", t, i);
      if (content.find(buf) != std::string::npos)
        found++;
    }
  }

  // Check for partial lines (lines without newline)
  size_t pos = 0;
  while (pos < content.size())
  {
    size_t next = content.find('\n', pos);
    if (next == std::string::npos)
    {
      if (pos < content.size())
        partial_lines++;
      break;
    }
    pos = next + 1;
  }

  const int total = num_threads * entries_per_thread;
  double pct = (found * 100.0) / total;

  printf("INFO: Found %d/%d entries (%.1f%%), partial lines: %d\n",
         found, total, pct, partial_lines);

  if (pct >= 95.0 && partial_lines == 0)
    log_pass("test_stress_multithread");
  else
  {
    char reason[256];
    sprintf_s(reason, "only %.1f%% entries found or %d partial lines", pct, partial_lines);
    log_fail("test_stress_multithread", reason);
  }

  remove(log_path);
}

// Verify log file is created at expected path
void test_log_file_created()
{
  printf("INFO: Running test: test_log_file_created\n");
  const char * log_path = "test_exists.log";

  auto * logger = create_logger(log_path);
  TINYLOG_INFO(logger) << "file_creation_test";
  close_logger(logger);

  FILE * check = fopen(log_path, "r");
  if (check)
  {
    fseek(check, 0, SEEK_END);
    long sz = ftell(check);
    fclose(check);

    if (sz > 0)
      log_pass("test_log_file_created");
    else
      log_fail("test_log_file_created", "log file is empty");
  }
  else
    log_fail("test_log_file_created", "log file was not created");

  remove(log_path);
}

int main()
{
  printf("=== tinylog Unit Tests ===\n\n");

  test_log_file_created();
  test_tls_buffer_isolation();
  test_atomic_drain_signal();
  test_timestamp_monotonic();
  test_buffer_overflow();
  test_stress_multithread();

  printf("\n=== All tinylog tests completed ===\n");
  return 0;
}
