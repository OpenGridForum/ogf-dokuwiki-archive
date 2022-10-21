// DRMAA signalling function is a dummy for Windows
// Therefore, this test suite tool is also a dummy in this case

#if !defined(WIN32)
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  /* various type definitions, like pid_t         */
#include <signal.h>     /* signal name macros, and the kill() prototype */
#endif

int main(int argc, char *argv[])
{
  if (argc != 2) return -1;

  #if !defined(WIN32)
  /* first, find my own process ID */
  pid_t my_pid = getpid();

  /* now that i got my PID, send myself the given signal. */
  kill(my_pid, atoi(argv[1]));

  while(1) {};
  #endif
  
  return 0;
}
