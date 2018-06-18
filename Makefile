CFLAGS = -Wall -std=c99

test_runner: test_functions.c test_functions.h test_runner.c
	gcc $(CFLAGS) -o test_runner test_runner.c test_functions.c
 
test_functions: test_functions.c test_runner.c test_functions.h test_runner.h
	gcc $(CFLAGS) test_functions.c test_runner.c -o test_functions

