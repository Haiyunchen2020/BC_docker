#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>


const int STACK_SIZE = 1024*1024;
const int BUF_SIZE = 1024;
const char *CONTAINER_HOSTNAME = "bcdocker";

void fail_and_exit(char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void run_app(char *app) {
  char *argv[] = {app, NULL};
  char *envp[] = {"PATH=/usr/bin:/bin", NULL};
  pid_t pid;

  pid = fork();

  if (pid == 0) {                               //child
    if (execve(app, argv, envp) ==  -1) {
      fail_and_exit("execve");
    }
  } else if (pid > 0)                           //parent
    if (waitpid(pid, NULL, 0) == -1) {
	fail_and_exit("waitpid(child, ....)");
      } else {
        umount("/proc");
	//	fail_and_exit("fork()");
      }
}

void help_and_exit(char *progname) {
  printf("Usage: %s run image appliaction\n", progname);
  exit(EXIT_FAILURE);
}

int run_setup_container(void *arg) {
  char hostname[BUF_SIZE];
  char **argv = (char **)arg;

  printf("child pid = %d\n", getpid());
  sethostname(CONTAINER_HOSTNAME, strlen(CONTAINER_HOSTNAME));
  gethostname(hostname, BUF_SIZE);
  printf("hostname = %s\n", hostname);
  if (chroot(argv[2]) == -1) {
    fail_and_exit("chroot");
  }
  chdir("/");

  if (mount ("proc", "proc" , "proc", 0, NULL) == -1) {
    fail_and_exit("mount proc");
  }
  
  run_app(argv[3]);
}

void run_container(int argc, char *argv[]) {
  
  char *stack, *stackTop;
  pid_t pid;
  
  stack = malloc(STACK_SIZE);
  if (stack == NULL) {
    fail_and_exit("malloc");
  }
  
  stackTop = stack + STACK_SIZE;
  pid = clone(run_setup_container, stackTop,  CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWCGROUP | SIGCHLD, (void *)argv);
  if (pid == -1) {
    fail_and_exit("clone(run_setup_container...)");
  }

  if (waitpid(pid, NULL, 0) == -1) {
    fail_and_exit("waitpid");
  }
}

int main (int argc, char *argv[]) {
  if(argc < 4) {
    help_and_exit(argv[0]);
  }

  if(strcmp (argv[1], "run") == 0) {
    run_container(argc, argv);
  } else {
    help_and_exit(argv[0]);
  }
  exit(EXIT_SUCCESS);
}

