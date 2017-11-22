#include <stdio.h>

void RemoveBlanks(char *str) {
   char *temp1 = str;
   while (*str) {
     *temp1 = *str++;
     if (*temp1 != ' ') {
        temp1++;
     }
   }
   *temp1 = '\0';
}

int main() {
   char s1[] = "  ", s2[] = "", s3[] = "a (b) c  ", s4[] = " ab ",
   s5[] = " aaab  ab abbb ";

   RemoveBlanks(s1);
   RemoveBlanks(s2);
   RemoveBlanks(s3);
   RemoveBlanks(s4);
   RemoveBlanks(s5);

   printf("|%s| |%s| |%s| |%s| |%s|\n", s1, s2, s3, s4, s5);

   return 0;
}
