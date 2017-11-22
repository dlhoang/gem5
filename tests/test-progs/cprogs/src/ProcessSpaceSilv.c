#include <stdio.h>

#define NUM 42

int *Recurse(int x) {
   int data = x;

   return x ? Recurse(x - 1) : &data;
}

int main() {
   int prm = NUM;
   int *v1 = Recurse(prm++);
   int *v2 = Recurse(prm++);
   int *v3 = Recurse(prm++);

   Recurse(prm + --prm); // Base: 6, Surcharge: 0

   printf("%d %d %d\n", *v1, *v2, *v3);

   return 0;
}
