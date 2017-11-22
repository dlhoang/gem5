#include <stdio.h>

int main(int argc, char **argv) {
   char *cp0;
   char *cp1;
   char **cpp0;
   char **cpp1;
   char **cpp2;

   cp0 = *(argv + 2) + 2;
   cp1 = "PURPLE" + 1;
   cpp0 = argv + 2;
   cpp1 = &cp0;
   cpp2 = cpp0 - 1;

   cpp2[-1][1] = cpp1[0][2] + 1;
   printf("%s\n", *cpp1);
   cpp1[0] = *cpp1 + 1;
   printf("%s\n", cp1);
   cp1 = *(cpp2 - 1) + 4;
   printf("%s\n", *cpp1);
   cpp1[0] = cp1;
   *(cpp2 - 1) = *cpp1;
   *(*(cpp2 - 1) - 2) = *(*(cpp0 - 2) + 2) + 1;
   printf("%s %s %s %s\n", cpp0[-2], *cpp1, *(cpp2 + 1), cp1);

   return 0;
}
