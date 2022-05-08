#pragma once

#include "spdlog/spdlog.h"

// clang-format off
#define LOG_INFO(...)  SPDLOG_LOGGER_INFO(spdlog::default_logger(), __VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_LOGGER_WARN(spdlog::default_logger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger(), __VA_ARGS__)
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(spdlog::default_logger(), __VA_ARGS__)

// log info per n times(include 1st), others log trace
#define LOG_PER(n, ...) \
    if (++cnt_[__LINE__] % n == 1) {    \
        LOG_INFO(__VA_ARGS__);          \
    } else {                            \
        LOG_TRACE(__VA_ARGS__);         \
    }


//#define LOG_INFO(...)
//#define LOG_WARN(...)
//#define LOG_ERROR(...)

