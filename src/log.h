#pragma once

#include "spdlog/spdlog.h"

#define LOG_INFO(...)  SPDLOG_LOGGER_INFO(spdlog::default_logger(), __VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_LOGGER_WARN(spdlog::default_logger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger(), __VA_ARGS__)


//#define LOG_INFO(...)
//#define LOG_WARN(...)
//#define LOG_ERROR(...)
