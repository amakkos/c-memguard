/*

  run-memguard.cc

  Example program to demonstrate MemGuard functionality.

  This program intentionally contains memory errors to showcase
  how MemGuard can detect and report them runtime.

  Compile with: g++ -std=c++17 -o test-memguard test-memguard.cc memguard.cc -lpthread
  Run with: ./test-memguard

*/

#include <stdio.h>
#include <errno.h>
#include <cstdlib>
#include <cstring>
// Include the MemGuard header for the last - to ensure macro overrides malloc/free/strcpy functions correctly
#include "memguard.h"

int main(int argc, char **argv)
{

  char *pointer;
  pointer = (char *) malloc(15);
  // testing sprintf - writing 12 bytes into 9 byte buffer
  sprintf(pointer, "Hello world!");
  // free(pointer);   // forgetting to free memory, so will show as memory leak
  printf("%s\n", pointer);

  // print memory guard report on exit
  MGPrint(15);
}
