#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PROCS 5

int main(int argc, char *argv[]) {
  int pid, proc, pidTotal = 0, reportFds[2];

  pipe(reportFds);

  /* Create PROCS children, call them child 0 through child PROCS-1 */
  for (proc = 0; proc < PROCS; proc++) {
      if (!(pid = fork())) {
         pid = getpid();
         write(reportFds[1], &pid, sizeof(pid));
         close(reportFds[0]);
         return 0;
       }
       else
         pidTotal += pid;
   }

   for (proc = 0; proc < PROCS; proc++) {
      wait(NULL);
   }

   close(reportFds[1]);

   for (proc = 0; proc < PROCS; proc++)
      if (0 < read(reportFds[0], &pid, sizeof(pid))) {
         printf("Good read\n");
         pidTotal -= pid;
      }

    if (0 == read(reportFds[0], &pid, sizeof(pid)))
        printf("After EOF on pipe, pidTotal is %d\n", pidTotal);

    return 0;
}
