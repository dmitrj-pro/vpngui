#ifndef DP_VPN_HEAD
#define DP_VPN_HEAD


#ifdef __cplusplus
	#include <string>
	#define EXTERNC extern "C"
#else
	#define EXTERNC
#endif

#include <unistd.h>

EXTERNC pid_t io_run (const char * command, int * infp, int * outfp);
EXTERNC void exec_cmd(const char * cmd);

#ifdef __cplusplus
	inline void exec(const std::string & cmd) {
		exec_cmd(cmd.c_str());
	}
	std::string runRead(const std::string & cmd);
#endif
#endif
