#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#define NFORKS 8
static pid_t pids[NFORKS];

static void sighand(int sig)
{
	exit(EXIT_SUCCESS);
}

int main()
{
	int i, ret;

	for (i = 0; i < NFORKS; i++) {
		pids[i] = fork();
		if (pids[i] < 0)
			exit(EXIT_FAILURE);
		else if (!pids[i]) {
			signal(SIGINT, sighand);
			fprintf(stderr, "fork %d: %d\n", i, getpid());
			for (;;) sleep(1);
			exit(EXIT_SUCCESS);
		}
	}

	//sleep(1);
	for (i = 0; i < NFORKS; i++) {
		kill(pids[i], SIGINT);
		while (pids[i] != waitpid(pids[i], &ret, 0))
			;
	}

	return 0;
}
