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
    fprintf(stderr, "Paniced at %s:%d: %s\n", __FILE__, __LINE__, s);          \
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
