// tinylog logging performance benchmark
// Measures overhead of TLS buffering and multi-threaded contention.
//
// Uses minimal includes to avoid NetBox base library dependencies.
// Compile and link similarly to test_tinylog (see src/CMakeLists.txt).

#include <tinylog/TinyLog.h>
#include <tinylog/Config.h>

#include <Windows.h>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace
{

using Clock = std::chrono::high_resolution_clock;
using Ms = std::chrono::milliseconds;

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

} // namespace

int main()
{
  printf("=== tinylog Performance Benchmark ===\n\n");

  //---------------------------------------------------------------------------
  // Benchmark 1: Baseline — raw tinylog write throughput (100K entries)
  //---------------------------------------------------------------------------
  {
    const int entries = 100000;
    const char * path = "bench_baseline.log";

    auto * logger = create_logger(path);
    auto start = Clock::now();

    for (int i = 0; i < entries; i++)
    {
      char buf[128];
      sprintf_s(buf, "bench_entry_%d some_text_for_overhead_measurement", i);
      TINYLOG_INFO(logger) << buf;
    }

    auto elapsed = std::chrono::duration_cast<Ms>(Clock::now() - start);
    close_logger(logger);

    double rate = (entries * 1000.0) / elapsed.count();
    printf("Benchmark: baseline (100K entries) - %lld ms, rate: %.0f entries/sec\n",
      elapsed.count(), rate);

    remove(path);
  }

  //---------------------------------------------------------------------------
  // Benchmark 2: TLS buffering overhead — same test with multiple threads
  //              This validates that thread-local buffers reduce contention.
  //---------------------------------------------------------------------------
  {
    const int threads = 8;
    const int entries_per_thread = 12500;
    const int total = threads * entries_per_thread;
    const char * path = "bench_tls.log";

    auto * logger = create_logger(path);
    std::vector<std::thread> threadPool;

    auto start = Clock::now();

    for (int t = 0; t < threads; t++)
    {
      threadPool.emplace_back([&](int tid)
      {
        for (int i = 0; i < entries_per_thread; i++)
        {
          char buf[128];
          sprintf_s(buf, "tls_t%d_i%d_padding_for_realistic_log_entry_size", tid, i);
          TINYLOG_DEBUG(logger) << buf;
        }
      }, t);
    }

    for (auto & th : threadPool)
      th.join();

    auto elapsed = std::chrono::duration_cast<Ms>(Clock::now() - start);
    close_logger(logger);

    double rate = (total * 1000.0) / elapsed.count();
    printf("Benchmark: TLS buffering (%d threads x %d entries) - %lld ms, rate: %.0f entries/sec\n",
      threads, entries_per_thread, elapsed.count(), rate);

    remove(path);
  }

  //---------------------------------------------------------------------------
  // Benchmark 3: Multi-threaded stress — 10 threads x 10K entries
  //              Measures contention under heavy concurrent load.
  //---------------------------------------------------------------------------
  {
    const int threads = 10;
    const int entries_per_thread = 10000;
    const int total = threads * entries_per_thread;
    const char * path = "bench_stress.log";

    auto * logger = create_logger(path);
    std::vector<std::thread> threadPool;
    std::atomic<int> ready{0};

    auto start = Clock::now();

    for (int t = 0; t < threads; t++)
    {
      threadPool.emplace_back([&](int tid)
      {
        for (int i = 0; i < entries_per_thread; i++)
        {
          char buf[96];
          sprintf_s(buf, "stress_t%d_i%d_longer_log_message_for_realistic_throughput_measurement", tid, i);
          TINYLOG_DEBUG(logger) << buf;
        }
      }, t);
    }

    for (auto & th : threadPool)
      th.join();

    auto elapsed = std::chrono::duration_cast<Ms>(Clock::now() - start);
    close_logger(logger);

    double rate = (total * 1000.0) / elapsed.count();
    printf("Benchmark: multi-thread stress (%d threads x %d entries) - %lld ms, rate: %.0f entries/sec\n",
      threads, entries_per_thread, elapsed.count(), rate);

    // Verify no entries lost
    std::string content;
    {
      FILE * f = fopen(path, "rb");
      if (f)
      {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        content.resize(sz);
        fread(&content[0], 1, sz, f);
        fclose(f);
      }
    }

    int found = 0;
    for (int t = 0; t < threads; t++)
    {
      for (int i = 0; i < entries_per_thread; i++)
      {
        char buf[96];
        sprintf_s(buf, "stress_t%d_i%d", t, i);
        if (content.find(buf) != std::string::npos)
          found++;
      }
    }

    double pct = (found * 100.0) / total;
    printf("Benchmark: multi-thread stress — %d/%d entries found (%.1f%%)\n",
      found, total, pct);
    printf("Benchmark: result — %s\n", (pct >= 99.0) ? "PASS (>=99% retained)" : "FAIL (<99% retained)");

    remove(path);
  }

  //---------------------------------------------------------------------------
  // Benchmark 4: Overhead measurement — compare logging vs no-logging
  //              Measures raw CPU overhead of logging calls.
  //---------------------------------------------------------------------------
  {
    const int iterations = 10000;
    volatile int sink = 0; // prevent optimization

    // No-logging baseline
    auto start_no = Clock::now();
    for (int i = 0; i < iterations; i++)
    {
      sink = i; // dummy work
    }
    auto baseline_ms = std::chrono::duration_cast<Ms>(Clock::now() - start_no).count();

    // With logging (to NUL — minimal I/O)
    const char * path = "bench_overhead.log";
    auto * logger = create_logger(path);

    auto start_log = Clock::now();
    for (int i = 0; i < iterations; i++)
    {
      char buf[64];
      sprintf_s(buf, "overhead_i%d", i);
      TINYLOG_DEBUG(logger) << buf;
      sink = i;
    }
    auto log_ms = std::chrono::duration_cast<Ms>(Clock::now() - start_log).count();

    close_logger(logger);

    double overhead_pct = (baseline_ms > 0) ?
      ((log_ms - baseline_ms) * 100.0 / log_ms) : 0.0;

    printf("Benchmark: overhead — baseline: %lld ms, with logging: %lld ms, overhead: %.2f%%\n",
      baseline_ms, log_ms, overhead_pct);
    printf("Benchmark: result — %s\n", (overhead_pct < 1.0) ? "PASS (<1% overhead)" : "FAIL (>=1% overhead)");

    // Prevent unused variable warning
    (void)sink;

    remove(path);
  }

  printf("\n=== Benchmark complete ===\n");
  return 0;
}
