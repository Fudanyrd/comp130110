#pragma once

#define RAND_MAX 32768

void test_init();

void alloc_test();
unsigned rand();
void srand(unsigned seed);

/** Entry point of a single-cpu test(i.e. not concurrent). */
void run_test(void);
