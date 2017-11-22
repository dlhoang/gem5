/* http://www.sanfoundry.com/c-program-tower-of-hanoi-using-recursion/
 *
 * C program for Tower of Hanoi using Recursion
 */

#include <stdio.h>

#define NUMBER 30

void towers(int, char, char, char);

int main()
{
   /*printf("The sequence of moves involved in the Tower of Hanoi are :\n");*/
   towers(NUMBER, 'A', 'C', 'B');
   return 0;
}

void towers(int num, char frompeg, char topeg, char auxpeg)
{
   if (num == 1)
   {
      /*printf("Move disk 1 from peg %c to peg %c\n", frompeg, topeg);*/
      return;
   }

   towers(num - 1, frompeg, auxpeg, topeg);
   /* printf("Move disk %d from peg %c to peg %c\n", num, frompeg, topeg);*/

   towers(num - 1, auxpeg, topeg, frompeg);
}

