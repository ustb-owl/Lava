#ifndef LAVA_DEBUG_H
#define LAVA_DEBUG_H

#include <cstdio>
#include <cstdlib>

#ifdef Is_True_On
// TRACE0: trace without message
#define TRACE0() \
  do { \
    printf("%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__); \
  } while (0)

// TRACE: trace with message
#define TRACE(fmt, ...) \
  do { \
    printf("%s:%d:%s: ", __FILE__, __LINE__, __FUNCTION__); \
    printf(fmt, ##__VA_ARGS__); \
  } while (0)

// DBG_ASSERT: output a warn if cond is false
#define DBG_WARN(cond, fmt, ...) \
  do { \
    if (!(cond)) { \
      printf("warning: %s:%d:%s\n  ", __FILE__, __LINE__, __FUNCTION__); \
      printf(fmt, ##__VA_ARGS__); \
    } \
  } while (0)

// DBG_ASSERT: output a warn and abort if cond is false
#define DBG_ASSERT(cond, fmt, ...) \
  do { \
    if (!(cond)) { \
      printf("error: %s:%d:%s() ", __FILE__, __LINE__, __FUNCTION__); \
      printf(fmt, ##__VA_ARGS__); \
      abort(); \
    } \
  } while (0)

// ERROR: output error message and break out
#define ERROR(fmt, ...)        \
  do {                          \
    printf("error: %s:%d:%s: ", __FILE__, __LINE__, __FUNCTION__);          \
    printf(fmt, ##__VA_ARGS__); \
    abort();                    \
  } while (0)

#else  // Is_True_On
#define TRACE0()
#define TRACE(fmt, ...)
#define DBG_WARN(cond, fmt, ...)
#define DBG_ASSERT(cond, fmt, ...)
#define ERROR(fmt, ...)
#endif // Is_True_On

#define REL_ASSERT(cond, fmt, ...) \
  do { \
    if (!(cond)) { \
      printf("error: %s:%d: ", __FILE__, __LINE__); \
      printf((fmt), __VA_ARGS); \
      abort(); \
    } \
  } while (0)

#endif //LAVA_DEBUG_H
