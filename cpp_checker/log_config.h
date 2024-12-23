#pragma once

// Log levels
#define LOG_INF    0
#define LOG_WRN    1
#define LOG_ERR    2

// Configure log output (can be changed to enable/disable different outputs)
#define LOG_TO_CONSOLE 1
#define LOG_TO_FILE    1

// Log file path (used when LOG_TO_FILE is enabled)
#define LOG_FILE_PATH L"logs/keyboard_checker.log"

// Minimum log level (logs below this level will be ignored)
#define MIN_LOG_LEVEL LOG_INF
