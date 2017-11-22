#include <assert.h>
#include <stdio.h>

int main() {
  short s1 = 30000;
  int count, step1 = 10000, step2 = 5536;

  assert(step1 > 0 && step2 > 0);
  for (count = 0; count < 4; count++) {
     s1 += step1;
     printf("%d ", s1);
  }
  printf("\n");

  for (count = 0; count < 4; count++) {
     s1 += step2;
     printf("%d ", s1);
  }
  printf("\n");

  return 0;
}
