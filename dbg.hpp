#pragma once
#ifndef DBG_HPP

struct {
  int LogLevel = 1;
} DBG;

#define __PrintError(...)                                 \
  do {                                                    \
    if (DBG.LogLevel < 1) break;                          \
    fprintf(stderr, "[E] (%s: %d) ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                         \
  } while (0)

#define __PrintInfo(...)                                  \
  do {                                                    \
    if (DBG.LogLevel < 2) break;                          \
    fprintf(stderr, "[I] (%s: %d) ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                         \
  } while (0)

#define __FprintfI(...)           \
  do {                            \
    if (DBG.LogLevel < 2) break;  \
    fprintf(__VA_ARGS__); \
  } while (0)

#define __FprintfE(...)           \
  do {                            \
    if (DBG.LogLevel < 1) break;  \
    fprintf(__VA_ARGS__); \
  } while (0)

#endif /* DBG_HPP */
