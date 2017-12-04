#include <stdio.h>
#include <stdlib.h>

int main() {
    int r = 50, c = 50, i, j, count;

    int *arr[r];
    for (i=0; i<r; i++) {
        arr[i] = (int *)malloc(c * sizeof(int));
    }

    count = 0;
    for (i = 0; i <  r; i++) {
        for (j = 0; j < c; j++) {
            arr[i][j] = ++count;
        }
    }

    for (i = 0; i < r; i++) {
        for (j = 0; j < c; j++) {
            printf("%d\n", arr[i][j]);
        }
    }
    printf("------   Second print   ------\n");

    for (i = r - 1; i >= 0; i--) {
        for (j = c - 1; j >= 0; j--) {
            printf("%d\n", arr[i][j]);
        }
    }

   return 0;
}
