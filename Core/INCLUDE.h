#pragma once

#define PortMemoryServer 612201805

#include <pthread.h>
#include "DEFINES.h"
#ifdef __cplusplus
	#include <sys/shm.h>
	#include <string>
#endif



#ifdef DP_DEBUG
	#define LOG(X)\
		printf(X);\
		printf("\n"); \
		fflush(stdout);
	#define LOG2(X, Y)\
		printf(X, Y);\
		printf("\n"); \
		fflush(stdout);
#else
	#define LOG(X)
	#define LOG2(X, Y)
#endif

enum TypeRequest{Allwais, None, Connect, Disconnect, Update_log, Exit, AddConfig, RmConfig, GetServers, GetProxy, AddProxy, RmProxy, SaveData, GetData};
enum CurrentStatus{Disconnected, Connecting, Connected};
//Connect file:type:proxy

#define ADD_SIZE 2048
#define ADD_SET_SIZE 256

typedef struct {
	char file[ADD_SIZE];
	int res_size;
	char ovpn_file[ADD_SET_SIZE];
	int ovpn_size;

	TypeRequest request;
	CurrentStatus status;

	pthread_mutex_t used;
	pthread_mutex_t lock_user;
	pthread_mutex_t lock_server;
} ServerStruct;

#ifdef __cplusplus
//ToDo
// Client * ClientOne::cl = nullptr;

	class Client{
		private:
			ServerStruct * data;
			int id;
		public:
			Client() {
				data = nullptr;
				id = shmget(PortMemoryServer, sizeof(ServerStruct), 0777);
				if (id < 0) {
					return;
				}
				data = (ServerStruct * ) shmat(id, NULL, 0);
			}
			inline void lock() {
				pthread_mutex_lock(&(data->used));
			}
			inline void unlock() {
				pthread_mutex_unlock(&(data->used));
			}
			inline void ExecVoid(TypeRequest req, const std::string & inf = "") {
				data->request = req;
				if (inf.size() > 0) {
					for (int i = 0; i < inf.size(); i++)
						data->file[i] = inf[i];
					data->file[inf.size()] = 0;
					data->res_size = inf.size();
				}
				pthread_mutex_unlock(&data->lock_server);
				pthread_mutex_lock(&data->lock_user);
			}
			inline std::string Exec(TypeRequest req, const std::string & inf = "") {
				ExecVoid(req, inf);
				std::string res;
				for (int i = 0 ; i < data->res_size; i++)
					res += data->file[i];
				return res;
			}
			inline void AExecVoid(TypeRequest req, const std::string & inf = "") {
				lock();
				ExecVoid(req, inf);
				unlock();
			}
			inline std::string AExec(TypeRequest req, const std::string & inf = "") {
				lock();
				std::string res = Exec(req, inf);
				unlock();
				return res;
			}
			inline ServerStruct * Core() { return data; }
	};
	class ClientOne{
		private:
			static Client * cl;
		public:
			ClientOne() {
				if (cl == nullptr)
					cl = new Client();
			}
			inline void lock() {
				cl->lock();
			}
			inline void unlock() {
				cl->unlock();
			}
			inline void ExecVoid(TypeRequest req, const std::string & inf = "") {
				cl->ExecVoid(req, inf);
			}
			inline std::string Exec(TypeRequest req, const std::string & inf = "") {
				return cl->Exec(req, inf);
			}
			inline void AExecVoid(TypeRequest req, const std::string & inf = "") {
				cl->AExecVoid(req, inf);
			}
			inline std::string AExec(TypeRequest req, const std::string & inf = "") {
				return cl->AExec(req, inf);
			}
			inline ServerStruct * Core() { return cl->Core(); }
	};
#endif
