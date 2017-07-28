#pragma once

#include <cstdint>
#include <string>

#include "Utils.h"

namespace tinylog {

class Buffer {
public:
    Buffer(uint32_t l_capacity);

    ~Buffer();

    int32_t TryAppend(apr_os_exp_time_t *pt_time, long u_sec, const char *pt_file, int i_line,
                      const char *pt_func, std::string &str_log_level, const char *pt_log);

    void Clear();
    apr_ssize_t Size() const;
    apr_ssize_t Capacity() const;
    int32_t Flush(int fd);

private:
    char *pt_data_;
    apr_ssize_t l_size_;
    apr_ssize_t l_capacity_;
};

} // namespace tinylog
