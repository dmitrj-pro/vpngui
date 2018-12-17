#include "Server.h"

int main() {
	MemServer server;
	server.Create();
	server.LoadStdFunction();
	server.Listen();
}
