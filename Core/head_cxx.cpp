#include "head.h"
#include <sys/wait.h>

using std::string;

std::string runRead(const std::string & cmd) {
	string res;
	char line[255];
	int out;
	int pid = io_run(cmd.c_str(), 0, &out);
	int readed;

	while(readed = read(out, line, 255))
		res += line;

	int status;
	waitpid(pid, &status, 0);
	return res;
}
