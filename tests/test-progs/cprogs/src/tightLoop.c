#include <stdio.h>

#define MAX 1000000

int main(int argc, char **argv) {
   int i, count = 0;
   int arr[MAX];

   for (i = 0; i < MAX; i++) {
      arr[i] = 0;
   }


   for (i = 1; i < MAX; i++) {
      if (count == MAX) {
         break;
      }
      arr[i] += 1;
      i--;
      count++;
   }
   printf("Value of arr[1]: %d\n", arr[1]);
   printf("Value of arr[%d]: %d\n", MAX - 1, arr[MAX - 1]);


   return 0;
}
