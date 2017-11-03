#include <stdio.h>
#include <unistd.h>

int main() {
   printf("Hello\n");
   fork();
   fork();
   fork();
   printf("Goodbye\n");
   return 0;
}
