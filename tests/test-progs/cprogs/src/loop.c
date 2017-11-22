#include <stdio.h>

#define MAX 1000000

int main(int argc, char **argv) {
   int i;
   int arr[MAX];

   for (i = 0; i < MAX; i++) {
      arr[i] = 0;
   }

   for (i = 1; i < MAX; i++) {
      arr[i] += arr[i - 1] + 2;
   }
   printf("Last value: %d\n", arr[MAX - 1]);

   return 0;
}
