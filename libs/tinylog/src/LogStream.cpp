#include <io.h>
#include <fcntl.h>
#include <cassert>

#include <nbsystem.h>
#include <tinylog/platform_win32.h>

#include <tinylog/LogStream.h>
#include <tinylog/Config.h>
// #include <Sysutils.hpp> // for DEBUG_PRINTF


namespace tinylog {

// TLS/FLS index — allocated once during first EnsureTls call across all threads.
// Dynamically resolves FlsAlloc (Vista+) via GetProcAddress; falls back to
// TlsAlloc on XP.  FlsAlloc provides per-thread cleanup callbacks, eliminating
// the leak that TlsAlloc cannot address.  Avoids C++11 thread_local which is
// unreliable in DLLs loaded via LoadLibrary.
DWORD LogStream::TlsIndex_ = TLS_OUT_OF_INDEXES;

static VOID WINAPI TlsBufferCleanup(PVOID data)
{
  nb_free(data);
}

// Lazily resolved function pointers — Vista+ APIs, not available on XP.
typedef DWORD (WINAPI * PFN_FlsAlloc)(PFLS_CALLBACK_FUNCTION);
typedef PVOID (WINAPI * PFN_FlsGetValue)(DWORD);
typedef BOOL  (WINAPI * PFN_FlsSetValue)(DWORD, PVOID);
typedef BOOL  (WINAPI * PFN_FlsFree)(DWORD);
static PFN_FlsAlloc pFlsAlloc = nullptr;
static PFN_FlsGetValue pFlsGetValue = nullptr;
static PFN_FlsSetValue pFlsSetValue = nullptr;
static PFN_FlsFree pFlsFree = nullptr;
static bool g_UseFls = false;
static bool g_CleanupTlsCalled = false;

LogStream::TlsLogBuffer & LogStream::EnsureTls()
{
  if (TlsIndex_ == TLS_OUT_OF_INDEXES)
  {
    // Resolve Fls* APIs dynamically — they don't exist on XP.
    if (!pFlsAlloc)
    {
      HMODULE hKernel32 = ::GetModuleHandleA("kernel32.dll");
      pFlsAlloc = reinterpret_cast<PFN_FlsAlloc>(
        ::GetProcAddress(hKernel32, "FlsAlloc"));
      pFlsGetValue = reinterpret_cast<PFN_FlsGetValue>(
        ::GetProcAddress(hKernel32, "FlsGetValue"));
      pFlsSetValue = reinterpret_cast<PFN_FlsSetValue>(
        ::GetProcAddress(hKernel32, "FlsSetValue"));
      pFlsFree = reinterpret_cast<PFN_FlsFree>(
        ::GetProcAddress(hKernel32, "FlsFree"));
      g_UseFls = (pFlsAlloc && pFlsGetValue && pFlsSetValue && pFlsFree);
    }
    if (g_UseFls)
      TlsIndex_ = pFlsAlloc(TlsBufferCleanup);
    else
      TlsIndex_ = ::TlsAlloc();
  }

  auto * buf = g_UseFls
    ? static_cast<TlsLogBuffer *>(pFlsGetValue(TlsIndex_))
    : static_cast<TlsLogBuffer *>(::TlsGetValue(TlsIndex_));

  if (!buf)
  {
    void * tls_mem = nb::chcalloc(sizeof(TlsLogBuffer));
    if (!tls_mem)
    {
      OutputDebugStringA("tinylog: FATAL: nb::chcalloc failed for TlsLogBuffer\n");
      std::abort();
    }
    buf = static_cast<TlsLogBuffer *>(tls_mem);
    if (g_UseFls)
      pFlsSetValue(TlsIndex_, buf);
    else
      ::TlsSetValue(TlsIndex_, buf);
  }
  return *buf;
}
LogStream::LogStream(FILE * file, pthread_mutex_t & mutex, pthread_cond_t & cond, std::atomic<bool> & drain_buffer) :
  front_buff_(nullptr),
  back_buff_(nullptr),
  file_(file),
  mutex_(mutex),
  cond_(cond),
  drain_buffer_(drain_buffer)
{
  void * fb_mem = nb::chcalloc(sizeof(Buffer));
  if (!fb_mem) { OutputDebugStringA("tinylog: FATAL: OOM for front Buffer\n"); std::abort(); }
  front_buff_ = new(fb_mem) Buffer(LOG_BUFFER_SIZE);
  void * bb_mem = nb::chcalloc(sizeof(Buffer));
  if (!bb_mem) { OutputDebugStringA("tinylog: FATAL: OOM for back Buffer\n"); std::abort(); }
  back_buff_ = new(bb_mem) Buffer(LOG_BUFFER_SIZE);
  // Initialize timestamp
  struct timeval tv_now;
  gettimeofday(&tv_now, nullptr);
  uint64_t us = static_cast<uint64_t>(tv_now.tv_sec) * 1000000ULL + tv_now.tv_usec;
  timestamp_us_.store(us, std::memory_order_relaxed);
}

LogStream::~LogStream()
{
  // DEBUG_PRINTF("end");
  // Flush any remaining log entries in the TLS buffer before closing the file
  auto & tls = EnsureTls();
  if (tls.used > 0)
  {
    InternalWrite(tls.buffer.data(), tls.used);
    tls.used = 0;
    // Immediately flush to file since background thread may be done
    WriteBuffer();
  }
  if (front_buff_)
  {
    front_buff_->~Buffer();
    nb_free(front_buff_);
    front_buff_ = nullptr;
  }
  if (back_buff_)
  {
    back_buff_->~Buffer();
    nb_free(back_buff_);
    back_buff_ = nullptr;
  }
  if (file_ != nullptr)
  {
    fclose(file_);
    file_ = nullptr;
  }
  // DEBUG_PRINTF("end");
}

void LogStream::CleanupTls()
{
  if (TlsIndex_ != TLS_OUT_OF_INDEXES)
  {
    if (g_UseFls && pFlsFree)
    {
      pFlsFree(TlsIndex_);
    }
    else
    {
      ::TlsFree(TlsIndex_);
    }
    TlsIndex_ = TLS_OUT_OF_INDEXES;
    pFlsAlloc = nullptr;
    pFlsGetValue = nullptr;
    pFlsSetValue = nullptr;
    pFlsFree = nullptr;
    g_UseFls = false;
    g_CleanupTlsCalled = true;
  }
}

bool LogStream::WasCleanupTlsCalled()
{
  return g_CleanupTlsCalled;
}

size_t LogStream::GetDroppedCount() const
{
  return dropped_count_.load(std::memory_order_relaxed);
}

size_t LogStream::Write(const char * data, size_t ToWrite)
{
  return FormattedWrite(data, ToWrite);
}

/*
 * Swap front buffer and back buffer.
 * This function should be locked.
 */
void LogStream::SwapBuffer()
{
  /*Buffer *pt_tmp = pt_front_buff_.release();
  pt_front_buff_.reset(pt_back_buff_.release());
  pt_back_buff_.reset(pt_tmp);*/
  std::swap(front_buff_, back_buff_);
}

/*
 * Write buffer data to log file.
 * This function should be locked.
 */
void LogStream::WriteBuffer()
{
  if (!file_)
    return;

  // Measure mutex hold time during file write
  auto start = std::chrono::steady_clock::now();
  front_buff_->Flush(file_);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
  
  // Log warning if write took too long (indicates mutex contention)
  auto ms = elapsed.count();
  if (ms > 10)
  {
    // Buffer copy optimization decision: if this warning appears frequently
    // in stress tests, implement copy-then-write to reduce mutex hold time
    char tmp[256];
    sprintf_s(tmp, "tinylog: WriteBuffer flush took %lld ms (threshold: 10ms) - mutex contention\n", ms);
    OutputDebugStringA(tmp);
  }
  
  front_buff_->Clear();

  /*std::string data;
  int ret = queue_->Pop(data);

  if (ret < 0)
    return;

  size_t n_write = 0;
  while ((n_write = fwrite(data.c_str(), 1, data.length(), file_)) != 0)
  {
    if ((n_write < 0) && (errno != EINTR))
    {
      // error
      break;
    }
    else if (n_write == data.length())
    {
      // All write
      break;
    }
    else if (n_write > 0)
    {
      // Half write
    }
  }*/
}

bool LogStream::EmergencyFlush()
{
  if (!pthread_mutex_tryenter(&mutex_))
  {
    OutputDebugStringA("tinylog: EmergencyFlush skipped (mutex busy)\n");
    return false;
  }

  size_t bytes_flushed = 0;

  if (file_ && front_buff_)
  {
    front_buff_->Flush(file_);
    bytes_flushed += front_buff_->Size();
    front_buff_->Clear();
  }

  if (drain_buffer_.load(std::memory_order_acquire))
  {
    SwapBuffer();
    if (file_ && front_buff_)
    {
      front_buff_->Flush(file_);
      bytes_flushed += front_buff_->Size();
      front_buff_->Clear();
    }
    drain_buffer_.store(false, std::memory_order_release);
  }

  pthread_mutex_unlock(&mutex_);

  char msg[256];
  _snprintf_s(msg, sizeof(msg), _TRUNCATE,
    "tinylog: EmergencyFlush complete (%zu bytes)\n", bytes_flushed);
  OutputDebugStringA(msg);
  return true;
}

void LogStream::SetFile(FILE * file)
{
  assert(file_ == nullptr);
  file_ = file;
}

size_t LogStream::FormattedWrite(const char * log_data, size_t to_write)
{
  // max length calculation. assume max line number is 999999
  // "2000-01-01 00:00:00.000 <file_name>:999999 <func_name> <level> <log_data>\n\0"
  // 24 + max(file, 10) + 1 + 6 + 1 + max(func, 16) + 1 + max(level, 10) + 1 + to_write + 2
  // 36 + max(file, 10) + max(func, 16) + max(level, 10) + to_write

  size_t Result = 0;

  UpdateBaseTime();

  auto & tls = EnsureTls();

  // Convert atomic timestamp to time components
  uint64_t us = timestamp_us_.load(std::memory_order_relaxed);
  time_t sec = static_cast<time_t>(us / 1000000ULL);
  int msec = static_cast<int>((us % 1000000ULL) / 1000);
  struct tm tm_now;
  localtime_s(&tm_now, &sec);

#define MAX(STR, DEF) nb::Max(((STR) ? strlen(STR) : 0ULL), (DEF))
  const size_t prefix_len = 36 + MAX(file_name_, 10ULL) +
    MAX(func_name_, 16ULL) + MAX(str_log_level_, 10ULL);
  const size_t append_len = prefix_len + to_write;
#undef MAX

  char * buf = nb::chcalloc(append_len);
  if (buf != nullptr)
  {
    int n_append = 0;
    if (file_name_ && *file_name_ && line_ > 0 && func_name_ && *func_name_ && str_log_level_ && *str_log_level_)
    {
      const int32_t line_number = line_ > 999999 ? 999999 : line_;

      n_append = sprintf_s(buf, append_len,
        "%d-%02d-%02d %02d:%02d:%02d.%.03d %10s:%3d %16s %10s %.*s\n",
        tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, msec,
        file_name_ ? file_name_ : "", line_number, func_name_ ? func_name_ : "", str_log_level_,
        static_cast<int>(to_write), log_data);
    }
    else
    {
      n_append = sprintf_s(buf, append_len,
        "%d-%02d-%02d %02d:%02d:%02d.%.03d %.*s\n",
        tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, msec,
        static_cast<int>(to_write), log_data);
    }

    if (n_append > 0)
    {
      // Use thread-local buffer to batch writes and reduce mutex contention
      if (tls.used + n_append > tls.buffer.size())
      {
        // TLS buffer full, flush existing content to shared buffer first
        if (tls.used > 0)
        {
          // Detect TLS corruption — tls.used should never exceed buffer size
          if (tls.used > tls.buffer.size())
          {
            tls.used = 0;
          }
          else
          {
            InternalWrite(tls.buffer.data(), tls.used);
            tls.used = 0;
          }
        }

        // If formatted line exceeds TLS buffer capacity, write it directly
        if (n_append > tls.buffer.size())
        {
          Result = InternalWrite(buf, n_append);
        }
        else
        {
          memcpy(tls.buffer.data(), buf, n_append);
          tls.used = n_append;
        }
      }
      else
      {
        // Append to TLS buffer
        memcpy(tls.buffer.data() + tls.used, buf, n_append);
        tls.used += n_append;
      }
    }
    nb_free(buf);
  }
  return Result;
}

size_t LogStream::InternalWrite(const char * log_data, size_t to_write)
{
  // Guard against corrupted to_write from TLS initialization failures.
  // A single formatted log line should never exceed 64 KB.
  if (to_write > 64 * 1024)
    return 0;

  const size_t Result = to_write;
  size_t n_append;

  pthread_mutex_lock(&mutex_);
  // DEBUG_PRINTF("ToWrite: %d", (int)to_write);
  while (to_write)
  {
    auto & buff = drain_buffer_.load(std::memory_order_acquire) ? back_buff_ : front_buff_;
    // append to the current buffer
    n_append = buff->TryAppend(log_data, to_write);
    if (n_append == 0)
    {
      if (drain_buffer_.load(std::memory_order_acquire))
      {
        // we are appending to the back_buff_ and there is no more space there
        // This is a rare situation - buffer overflow under heavy load
        // Drop this log entry and increment dropped counter
        size_t dropped = dropped_count_.fetch_add(1, std::memory_order_relaxed);
        
        // Log warning every 100 dropped entries to avoid log spam
        if (dropped % 100 == 0)
        {
          // Signal background thread to flush
          pthread_cond_signal(&cond_);
        }
        
        // Skip this log entry
        to_write = 0;
      }
      else
      {
        drain_buffer_.store(true, std::memory_order_release);
      }
    }
    else
    {
      log_data += n_append;
      to_write -= n_append;
    }
  }

  if (drain_buffer_.load(std::memory_order_acquire))
  {
    pthread_cond_signal(&cond_);
  }
  pthread_mutex_unlock(&mutex_);

  return Result;
}

LogStream & LogStream::operator<<(const char * log_data)
{
  FormattedWrite(log_data, strlen(log_data));

  return *this;
}

#if 0
LogStream & LogStream::operator<<(const std::string & ref_log)
{
  FormattedWrite(ref_log.c_str(), ref_log.size());
/*
  UpdateBaseTime();

  auto & tls = EnsureTls();

  char buff[256]{};
  int n_append = sprintf(buff, "%d-%02d-%02d %02d:%02d:%02d.%.03ld %s %d %s %s %s\n",
      tm_base_.tm_year + 1900, tm_base_.tm_mon + 1, tm_base_.tm_mday,
      tm_base_.tm_hour, tm_base_.tm_min, tm_base_.tm_sec, (long)tv_base_.tv_usec / 1000,
      file_name_, line_, func_name_, str_log_level_.c_str(),
      ref_log.c_str());

  std::string log = buff;

  queue_->Push(log);
*/
  return *this;
}
#endif 0

void LogStream::UpdateBaseTime()
{
  struct timeval tv_now;
  gettimeofday(&tv_now, nullptr);
  uint64_t us = static_cast<uint64_t>(tv_now.tv_sec) * 1000000ULL + tv_now.tv_usec;
  timestamp_us_.store(us, std::memory_order_relaxed);
}

void LogStream::SetPrefix(const char * file_name, int32_t line, const char * func_name, Utils::LogLevel log_level)
{
  file_name_ = tinylog::past_last_slash(file_name);
  line_  = line;
  func_name_ = func_name;

  switch (log_level)
  {
  case Utils::LEVEL_DEBUG:
    str_log_level_ = "[DEBUG  ]";
    break;
  case Utils::LEVEL_INFO:
    str_log_level_ = "[INFO   ]";
    break;
  case Utils::LEVEL_WARNING:
    str_log_level_ = "[WARNING]";
    break;
  case Utils::LEVEL_ERROR:
    str_log_level_ = "[ERROR  ]";
    break;
  case Utils::LEVEL_FATAL:
    str_log_level_ = "[FATAL  ]";
    break;
  default:
    str_log_level_ = "[INFO   ]";
    break;
  }
}

} // namespace tinylog
