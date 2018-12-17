#include <iostream>
#include "INCLUDE.h"
#include <sys/types.h>
#include <sys/shm.h>
#include <cstring>
#include <unistd.h>

using std::string;
using std::cout;
using std::cin;

#define CMD(X) \
	std::cout << "CMD: " << X << "\n";

Client * ClientOne::cl = nullptr;

int main() {
	ClientOne server;

	while (true) {
		string line;
		cout << "Input command: ";
		getline(cin, line);

		string cmd = "";
		string addr = "";

		for (int i = 0; i < line.size(); i++) {
			if (line[i] == ' ') {
				cmd = line.substr(0, i);
				addr = line.substr(i+1);
				break;
			}
		}
		if (cmd.size() == 0 || cmd == "status" || cmd == "STATUS") {
			CMD("STATUS");

			if (server.Core()->status == Disconnected)
				std::cout << "Disconnected:\n";
			if (server.Core()->status == Connected)
				std::cout << "Connected:\n";
			if (server.Core()->status == Connecting)
				std::cout << "Connecting:\n";

			std::cout << server.AExec(Update_log) << "\n";
		}

		if (cmd.size() > 0 && ( cmd == "CONNECT" || cmd == "connect")) {
			CMD("CONNECT");
			server.AExecVoid(Connect, addr);
			cout << "OK\n";
		}
		if (cmd.size() > 0 && (cmd == "pdelete" || cmd == "PDELETE")) {
			CMD("Delete proxy");
			server.AExecVoid(RmProxy, addr);
		}
		if (line == "exit" || line == "EXIT") {
			CMD("EXIT");
			server.AExecVoid(Exit);
			std::cout << "OK\n";
			break;
		}
		if (line == "list" || line == "LIST") {
			CMD("LIST");

			server.lock();
			ServerStruct * data =server.Core();

			data->request = GetServers;
			while (true) {
				std::cout << "\nNext PART:\n";
				pthread_mutex_unlock(&data->lock_server);
				pthread_mutex_lock(&data->lock_user);
				for (int i = 0 ; i < data->res_size; i++) {
					std::cout << data->file[i];
				}
				if (data->request != None)
					continue;
				break;
			}

			server.unlock();
		}

		if (cmd == "set" || cmd == "SET") {
			CMD("SET");
			server.AExecVoid(SaveData, addr);
			std::cout << "OK\n";
		}
		if (cmd == "get" || cmd == "GET") {
			CMD("GET");
			std::cout << server.AExec(GetData, addr) << "\n";

		}
		if (line == "plist" || line == "PLIST") {
			CMD("PLIST");
			server.lock();
			ServerStruct * data =server.Core();

			data->request = GetProxy;
			while (true) {
				std::cout << "\nNext PART:\n";
				pthread_mutex_unlock(&data->lock_server);
				pthread_mutex_lock(&data->lock_user);
				for (int i = 0 ; i < data->res_size; i++) {
					std::cout << data->file[i];
				}
				if (data->request != None)
					continue;
				break;
			}
			server.unlock();
		}

		if (cmd.size() > 0 && (cmd == "add" || cmd == "ADD")) {
			CMD("ADD");
			server.AExecVoid(AddConfig, addr);
			cout << "OK\n";
		}
		if (cmd.size() > 0 && (cmd == "proxy" || cmd == "PROXY")) {
			CMD("PROXY");
			server.AExecVoid(AddProxy, addr);
			cout << "OK\n";
		}

		if ((line == "disconnect") || (cmd.size() > 0 && ( cmd == "DISCONNECT" || cmd == "disconnect"))) {
			CMD("DISCONNECT");
			server.AExecVoid(Disconnect);
			cout << "OK\n";
		}
	}
}
