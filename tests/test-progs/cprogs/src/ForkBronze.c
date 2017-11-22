
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FORKS 30

int main() {
    int i, pid;

    for (i = 0; i < FORKS; i++) {
        if (!(pid = fork())) {
            if (!(i % 10))
                sleep(2);
            printf("%d\n", i);
            i = FORKS; // Base: 3, Surcharge: 0
        }
        else
            wait(&pid); // Base: 4, Surcharge: 0
    }

    return 0;
}
