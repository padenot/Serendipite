#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#define MACRO_BEG do {
#define MACRO_END } while(0);

enum log_level {
  LOG_FATAL,    /* this error will kill the program */
  LOG_CRITICAL, /* nasty error */
  LOG_WARNING,  /* something is getting wrong */
  LOG_OK,       /* working */
  LOG_DEBUG,    /* normal output message */
  COLOR_RESET,  /* resets the color to user default */
  COLORS_SIZE
};

typedef enum {
  OK = 0,
  ERROR = -1
} _error_codes;

#ifdef ENABLE_COLORS
static const char* COLORS[COLORS_SIZE] = {
  "\e[1;91m", /* FATAL bold red. */
  "\e[0;91m", /* CRITICAL red. */
  "\e[0;93m", /* WARNING yellow. */
  "\e[0;92m", /* OK green. */
  "",         /* DEBUG normal. */
  "\e[0m"     /* reset. */
};
#else
static const char* COLORS[COLORS_SIZE] = {
  "",
  "",
  "",
  "",
  "",
  ""
};
#endif

#define SYSCALL(x)                                                             \
        MACRO_BEG                                                              \
        if(x == -1) {                                                          \
          fprintf(stderr, "%s#! SYSCALL FAILED : %s:%d:%s:%s%s\n",             \
                  COLORS[LOG_FATAL], __FILE__, __LINE__,                       \
                  #x, strerror(errno), COLORS[COLOR_RESET]);                   \
          abort();                                                             \
        }                                                                      \
        MACRO_END

#ifdef DEBUG
  #ifndef LOG_LEVEL
    #define LOG_LEVEL LOG_DEBUG
  #endif

  /* The current indent level */
  static size_t log_indent_level = 0;
  /* Indents the log by one level */
  #define LOG_INDENT log_indent_level++;
  /* Unindents the log by one level */
  #define LOG_UNINDENT ASSERT(log_indent_level-- > (size_t)0,   \
                             "Log indent level is now negative");

  /* Logs to stdout, using colors for the levels */
  #define LOG(level, ...)                                                      \
    MACRO_BEG                                                                  \
      if(level <= LOG_LEVEL) {                                                 \
        size_t i;                                                              \
        fprintf(stdout, "%s", COLORS[level]);                                  \
        for(i=0; i < log_indent_level ; i++) {                                 \
          fprintf(stdout, "\t");                                               \
        }                                                                      \
        if (pthread_self() != 0) {                                             \
          fprintf(stdout, "[%ld] ", pthread_self());                           \
        }                                                                      \
        fprintf(stdout, __VA_ARGS__);                                          \
        fprintf(stdout, "\n");                                                 \
        fprintf(stdout, "%s", COLORS[COLOR_RESET]);                            \
      }                                                                        \
    MACRO_END

  /* If ASSERT_FATAL is defined, a failing asserts will kill the program. */
  #ifdef ASSERT_FATAL
    #define ASSERT(test, msg)                                                  \
      if (!test) {                                                             \
        LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d:%s\n",                       \
            __FILE__, __LINE__, #msg);                                         \
        abort();                                                               \
      }
  #else
    #define ASSERT(test, msg)                                                  \
      if (test == 0) {                                                         \
        LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d:%s\n",                       \
            __FILE__, __LINE__, #msg);                                         \
      }
  #endif

  /**
   * @brief Assert that a value is in a range. Strict comparison version.
   *
   * @param x The value to check.
   * @param ref The reference value.
   * @param thres The threshold.
   */
  #define BOUND(x, ref, thres)                                                 \
    if (fabs(x - ref < thres)) {                                               \
      LOG(LOG_CRITICAL,                                                        \
               "#! ASSERT FAILED %s:%d, %d should be > %d and < %d ",          \
               __FILE__, __LINE__, x, ref-thres, ref+thres);                   \
    }

  /**
   * @brief Assert that a value is in a range. Inclusive comparison version.
   *
   * @param x The value to check.
   * @param ref The reference value.
   * @param thres The threshold.
   */
  #define BOUNDI(x, ref, thres)                                                \
    if (fabs(x - ref) <= thres) {                                              \
      LOG(LOG_CRITICAL,                                                        \
               "#! ASSERT FAILED %s:%d, %d should be >= %d and <= %d ",        \
               __FILE__, __LINE__, x, ref-thres, ref+thres);                   \
    }

  /**
   * @brief Assert that a value is zero.
   *
   * @param x The value to check.
   */
  #define ZERO(x)                                                              \
    if (x != 0) {                                                              \
      LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d, %d should be 0",              \
          __FILE__, __LINE__, x);                                              \
    }

  /**
   * @brief Assert that a value is positive. Strict comparison version.
   *
   * @param x The value to check.
   */
  #define POS(x)                                                               \
    if (x < 0) {                                                               \
      LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d, %d should be > 0",            \
                                   __FILE__, __LINE__, x);                     \
    }
  /**
   * @brief Assert that a value is negative. Strict comparison version
   *
   * @param x The value to check.
   */
  #define NEG(x)                                                               \
    if (x > 0) {                                                               \
      LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d, %d should be < 0.",           \
                                   __FILE__, __LINE__, x);                     \
    }

  /**
   * @brief Assert that a value is positive. Inclusive comparison version.
   *
   * @param x The value to check.
   */
  #define POSI(x)                                                              \
    if (x <= 0) {                                                              \
      LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d, %d should be >= 0",           \
                                 __FILE__, __LINE__, x);                       \
    }

  /**
   * @brief Assert that a value is negative. Inclusive comparison version
   *
   * @param x The value to check.
   */
  #define NEGI(x)                                                              \
    if (x >= 0) {                                                              \
      LOG(LOG_CRITICAL, "#! ASSERT FAILED %s:%d, %d should be <= 0.",          \
                                  __FILE__, __LINE__, x);                      \
    }
#else
    #define ASSERT(a...);
    #define LOG(a...);
    #define BOUND(a...);
    #define BOUNDI(a...);
    #define ZERO(a...);
    #define POS(a...);
    #define NEG(a...);
    #define POSI(a...);
    #define NEGI(a...);
#endif /* DEBUG */

/* UNUSED(arg) to remove compiler warning for a parameter
   void func() UNUSED to remove compiler warning for a function */
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
# define FUNUSED __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

/**
 * Traces the allocations and the deallocations.
 * if PRINT_ALLOC is defined, a message is logged every time an allocation
 * occurs.
 * at a normal exit of the program, the balance of alloc and dealloc is
 * outputed.
 */
#ifdef COUNT_ALLOC
  static size_t alloc_count = 0;

  static void trace_exit_handler() FUNUSED;
  static void trace_exit_handler()
  {
    if (alloc_count) {
      LOG(LOG_CRITICAL,
                "Alloc balance: %zu. Memory leaks detected.",
                alloc_count);
    } else {
      LOG(LOG_OK,
               "Alloc balance: %zu. No memory leaks.",
               alloc_count);
    }
    exit(0);
  }

  static size_t alloc_balance() FUNUSED;
  static size_t alloc_balance()
  {
    return alloc_count;
  }

  #define set_exit_handler()                                                   \
    if(atexit(trace_exit_handler)) {                                           \
      LOG(LOG_FATAL, "Couldn't set exit function.\n");                         \
      abort();                                                                 \
    }
  #ifdef PRINT_ALLOC
    #define ALLOC(x)                                                           \
    ALLOC_IMPL(x);                                                             \
    LOG(LOG_DEBUG, "Alloc in %s.", __func__);                                  \
    alloc_count++;
  #else
    #define ALLOC(x)                                                           \
    ALLOC_IMPL(x);                                                             \
    alloc_count++;
  #endif

  #ifdef PRINT_ALLOC
    #define DELETE(x)                                                          \
      DELETE_IMPL(x);                                                          \
      LOG(LOG_DEBUG, "Delete in %s.", __func__);                               \
      alloc_count--;
  #else
    #define DELETE(x)                                                          \
      DELETE_IMPL(x);                                                          \
      alloc_count--;
  #endif
#else
  #define ALLOC(x) ALLOC_IMPL(x)
  #define DELETE(x) DELETE_IMPL(x)
  #define set_exit_handler()
#endif

#define ALLOC_IMPL(x)                                                          \
  malloc(x);                                                                   \

#define DELETE_IMPL(x) \
  free(x);                                                                     \

#endif /* MACROS_H */
