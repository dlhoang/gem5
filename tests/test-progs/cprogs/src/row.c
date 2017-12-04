#include <stdio.h>
#include <stdlib.h>

int main() {
    int r = 100, c = 100, i, j, count;

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

    for (i = 0; i < r; i++) {
        for (j = 0; j < c; j++) {
            printf("%d\n", arr[i][j]);
        }
    }

   return 0;
}
