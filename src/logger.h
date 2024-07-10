#pragma once

#include <loguru.hpp>

#define LOG_INFO(format, ...)   LOG_F(INFO,      format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)   LOG_F(WARNING,   format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)  LOG_F(ERROR,     format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)  LOG_F(FATAL,     format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)  LOG_F(1,         format, ##__VA_ARGS__)
