#include <assert.h>
#include <locale.h>
#include <stdio.h>

#define NUMBER 20

unsigned long factorial(int num) {
   assert(num >= 0);

   if (num == 0 || num == 1) {
      return 1;
   }
   return num * factorial(num - 1);
}

int main(int argc, char **argv) {
   setlocale(LC_NUMERIC, "");
   printf("Factorial of %d is %'lu\n", NUMBER, factorial(NUMBER));
   return 0;
}
