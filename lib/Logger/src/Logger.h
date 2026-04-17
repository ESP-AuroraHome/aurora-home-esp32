#pragma once

#include <Arduino.h>

#include "Config.h"

/**
 * @file Logger.h
 * @brief Macros de log a niveaux (ERROR/WARN/INFO/DEBUG).
 *
 * Le niveau est defini par AURORA_LOG_LEVEL dans Config.h (0..4). Les logs
 * au-dessus du niveau courant sont compiles en no-op (zero cout runtime).
 */

#define AURORA_LOG_NONE 0
#define AURORA_LOG_ERROR 1
#define AURORA_LOG_WARN 2
#define AURORA_LOG_INFO 3
#define AURORA_LOG_DEBUG 4

#ifndef AURORA_LOG_LEVEL
#define AURORA_LOG_LEVEL AURORA_LOG_INFO
#endif

#define AURORA_LOG_IMPL_(tag, fmt, ...)                      \
    do {                                                     \
        Serial.printf("[" tag "] " fmt "\n", ##__VA_ARGS__); \
    } while (0)

#if AURORA_LOG_LEVEL >= AURORA_LOG_ERROR
#define LOG_ERROR(fmt, ...) AURORA_LOG_IMPL_("ERR ", fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) \
    do {                    \
    } while (0)
#endif

#if AURORA_LOG_LEVEL >= AURORA_LOG_WARN
#define LOG_WARN(fmt, ...) AURORA_LOG_IMPL_("WARN", fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) \
    do {                   \
    } while (0)
#endif

#if AURORA_LOG_LEVEL >= AURORA_LOG_INFO
#define LOG_INFO(fmt, ...) AURORA_LOG_IMPL_("INFO", fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) \
    do {                   \
    } while (0)
#endif

#if AURORA_LOG_LEVEL >= AURORA_LOG_DEBUG
#define LOG_DEBUG(fmt, ...) AURORA_LOG_IMPL_("DBG ", fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) \
    do {                    \
    } while (0)
#endif
