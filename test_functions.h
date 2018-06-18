#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*func)(void);

// write(1, "PASS", 5);

#define lab_assert(expr_bool, fail_string) ({\
  if (!(expr_bool)) {\
    fprintf(stderr, "%s:%d: %s", __FILE__, __LINE__, fail_string);\
    exit(1);\
  }\
})

#define lab_printf(fmt, ...) printf("%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define lab_fprintf(pFILE, fmt, ...) fprintf(pFILE, "%s : " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define TEST (printf("%s\n", __func__));
#define TIMEOUT(SECONDS) ({                                       \
  lab_fprintf(stdout, "-> CUSTOM TIMEOUT SET TO %ds 🕒", SECONDS);\
  alarm(0);                                                       \
  alarm(SECONDS);                                                 \
})

void func1(void);
void func2(void);
void func3(void);
void func4(void);
void func5(void);
void funcz(void);

static func test_list[] = {func1, func2, func3, func4, func5, funcz};

#include "test_runner.h"