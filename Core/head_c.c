#include "head.h"
#include <fcntl.h>


#define DP_DEBUG
#ifdef DP_DEBUG
	#include <stdio.h>
#endif

pid_t io_run (const char * command, int * infp, int * outfp) {
	#ifdef DP_DEBUG
		printf("Pipe %s\n", command);
		fflush(stdout);
	#endif
	int p_stdin[2], p_stdout[2];
	pid_t pid;
	if (pipe(p_stdin) == -1) {
		#ifdef DP_DEBUG
			printf("Fail to create stdin pipe\n");
			fflush(stdout);
		#endif
		return -1;
	}
	if (pipe(p_stdout) == -1) {
		#ifdef DP_DEBUG
			printf("Fail to create stdout pipe\n");
			fflush(stdout);
		#endif
		close(p_stdin[0]);
		close(p_stdin[1]);
	}
	pid = fork();

	if (pid < 0) {
		close(p_stdin[0]);
		close(p_stdin[1]);
		close(p_stdout[0]);
		close(p_stdout[1]);
		return pid;
	} else if (pid == 0) {
		close(p_stdin[1]);
		dup2(p_stdin[0], 0);
		close(p_stdout[0]);
		dup2(p_stdout[1], 1);
		dup2(open("/dev/null", O_WRONLY), 2);

		for (int i = 3; i < 4096; ++i)
			close(i);
		setsid();
		execl("/bin/bash", "sh", "-c", command, NULL);
		_exit(1);
	}

	close(p_stdin[0]);
	close(p_stdout[1]);
	if (infp == NULL)
		close(p_stdin[1]);
	else
		*infp = p_stdin[1];
	if (outfp == NULL)
		close(p_stdout[0]);
	else
		*outfp = p_stdout[0];

	return pid;
}


void exec_cmd(const char * cmd) {
	#ifdef DP_DEBUG
		printf ("Exec: %s\n", cmd);
		fflush(stdout);
	#endif
	system(cmd);
}
