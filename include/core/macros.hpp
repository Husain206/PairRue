#pragma once

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define MIN3(a, b, c) (((a) < (b)) ? ((b) < (c)) ? ((a) : (b) : ((b) < (c)) ? (b) : (c))
#define MAX3(a, b, c) (((a) > (b)) ? ((b) > (c)) ? ((a) : (b) : ((b) > (c)) ? (b) : (c))

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define PANIC(s)                                                               \
  do {                                                                         \
    fprintf(stderr, "\033[31mPaniced at %s:%d: %s\033[0m\n", __FILE__, __LINE__, s);          \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define PANICF(...) \
  do {                                                                         \
    fprintf(stderr, "\033[31mPaniced at %s:%d: \033[0m", __FILE__, __LINE__);          \
    fprintf(stderr, __VA_ARGS__); \
    exit(EXIT_FAILURE);                                                        \
  } while (0)


#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      PANIC("assertion failed: " #cond);                                       \
    }                                                                          \
  } while (0)

#define UNREACHABLE                                                            \
  do {                                                                         \
    PANIC("unreachable code");                                                 \
  } while (0)


#define SWITCH(x) strview __S__ = x; if(0)
#define CASE(y) }else if(__S__ == y){ 
#define DEFAULT } else {
#define DO {
#define END }
