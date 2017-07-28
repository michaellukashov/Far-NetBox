//
// Created by ouyangliduo on 2016/12/13.
//

#include <time.h>

#include "TinyLog.h"
#include "Config.h"

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
bool g_already_swap = false;

static void *ThreadFunc(void *pt_arg)
{
    TinyLog *pt_tinylog = (TinyLog *)pt_arg;
    pt_tinylog->MainLoop();

    return NULL;
}

TinyLog::TinyLog()
{
    pt_logstream_ = new LogStream();
    e_log_level_ = Utils::INFO;
    b_run_ = true;
    pthread_create(&tid_, NULL, ThreadFunc, this);
}

TinyLog::~TinyLog()
{
    b_run_ = false;
    pthread_join(tid_, NULL);
    delete(pt_logstream_);
}

void TinyLog::SetLogLevel(Utils::LogLevel e_log_level)
{
    e_log_level_ = e_log_level;
}

Utils::LogLevel TinyLog::GetLogLevel()
{
    return e_log_level_;
}

LogStream& TinyLog::GetLogStream(const char *pt_file, int i_line, const char *pt_func, Utils::LogLevel e_log_level)
{
    pt_logstream_->SetPrefix(pt_file, i_line, pt_func, e_log_level);
    return *pt_logstream_;
}

int32_t TinyLog::MainLoop()
{
    struct timespec st_time_out;

    while (b_run_)
    {
        st_time_out.tv_sec = pt_logstream_->tv_base_.tv_sec + TIME_OUT_SECOND;
        st_time_out.tv_nsec = pt_logstream_->tv_base_.tv_usec * 1000;

        pthread_mutex_lock(&g_mutex);

        while (!g_already_swap)
        {
            if (pthread_cond_timedwait(&g_cond, &g_mutex, &st_time_out) == ETIMEDOUT)
            {
                pt_logstream_->SwapBuffer();
                pt_logstream_->UpdateBaseTime();
                break;
            }
        }

        if (g_already_swap)
        {
            g_already_swap = false;
        }

        pt_logstream_->WriteBuffer();

        pthread_mutex_unlock(&g_mutex);
    }

    return 0;
}