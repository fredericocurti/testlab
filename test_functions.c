#include "test_functions.h"

// std err
void func1() {
  TEST
  lab_printf("func1 rodou!");
  lab_assert(1 == 0, "1 != 0!");
}

void funcz(){
  TEST
  lab_fprintf(stderr, "eeqew \n wew \n qwe\n");
  lab_assert(1 == 1, "ae");
}

// timeout
void func2() {
  TEST
  TIMEOUT(3); // api pra mudar timeout

  sleep(7);
  lab_fprintf(stderr, "saída de erros!");
  lab_assert(1 == 1, "vixi!");
}

// segfault!
void func3() {
  TEST
  char *s = "hello world";
  *s = 'H';
  lab_assert(2 == 1, "ai n deu né\n");
}

// std true
void func4() {
  TEST
  lab_printf("func4 rodandoo");
  lab_assert(1 == 1, "1 != 0!");
}

// while 1
void func5() {
  TEST
  lab_assert(1 == 2, "TROLLEI");
  while(1){
    sleep(1);
  }
}