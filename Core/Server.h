#pragma once
#include "INCLUDE.h"
#include <DPLib.conf.h>
#include <pthread.h>

using __DP_LIB_NAMESPACE__::Map;
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::UInt;
using __DP_LIB_NAMESPACE__::Pair;

#define buf_size 1024


#ifdef DP_DEBUG
	#define NONENC
	#undef ENCCONF
#else
	#define ENCCONF
	#undef NONENC
#endif

#define TMP_LOG_FILE "tmp_log.txt"
#define TMP_CONF_FILE "fuck.ovpn"

#ifdef ENCCONF
	#include "SCH1.h"
#endif

String GetFileName(const String & file);
String GetFilePath(const String & file);
void CP(const String & before, const String & after);

class ConfigDataBase{
	public:
		struct VPNConfig {
			String file;
			enum class ConfigType{None, LoginPassword, KeysFile};
			ConfigType type;
			String login;
			String password;
		};
		// IN: Name:File:Type:?Login:?Password
		static Pair<String, VPNConfig> parseString(const String & conf);
		static String toStringConf(const Pair<String, VPNConfig> & conf);
		static inline String toStringType(VPNConfig::ConfigType type) {
			switch (type) {
				case VPNConfig::ConfigType::None:
					return "none";
				case VPNConfig::ConfigType::KeysFile:
					return "key";
				case VPNConfig::ConfigType::LoginPassword:
					return "loginpassword";
			}
			return "UNKNOW";
		}
		static inline VPNConfig::ConfigType parseType(const String & conf) {
			if (conf == "none")
				return VPNConfig::ConfigType::None;
			if (conf == "key")
				return VPNConfig::ConfigType::KeysFile;
			if (conf == "loginpassword")
				return VPNConfig::ConfigType::LoginPassword;
			return VPNConfig::ConfigType::None;
		}
		struct ProxyConfig {
			String server;
			UInt port;
			enum class ProxyType{Http, Socks};
			ProxyType type;
		};
		static inline String toStringType(ProxyConfig::ProxyType type) {
			switch (type) {
				case ProxyConfig::ProxyType::Http:
					return "http";
				case ProxyConfig::ProxyType::Socks:
					return "socks";
				default:
					return "none";
			}
		}
		static inline ProxyConfig::ProxyType parseStringProxyType(const String & type) {
			if (type == "http")
				return ProxyConfig::ProxyType::Http;
			if (type == "socks")
				return ProxyConfig::ProxyType::Socks;

			return ProxyConfig::ProxyType::Http;
		}

		// IN: Name|server|port|type
		static Pair<String, ProxyConfig> parseStringProxy(const String & conf);
		static String toStringProxy(const Pair<String, ProxyConfig> & conf);
		enum class ProtoType{None, TCP, UDP};

		static inline ProtoType parseProto(const String & type) {
			if (type == "tcp")
				return ProtoType::TCP;
			if (type == "udp")
				return ProtoType::UDP;
			return ProtoType::None;
		}
		static inline String toStringProto(ProtoType type) {
			switch (type) {
				case ProtoType::TCP:
					return "tcp";
				case ProtoType::UDP:
					return "udp";
				default:
					return "tcp";
			}
		}

	private:
		#ifdef ENCCONF
			ENC::SCH1 _crypt = ENC::SCH1(ENC_PASSWORD);
		#endif
		Map<String, VPNConfig> _conf;
		Map<String, ProxyConfig> _proxyes;
		List<String> tmp_file;
		Map<String, String> _save_data;
		void Save();
		void Load();

		inline void Add(const String & name, const VPNConfig & conf) {
			_conf[name] = conf;
		}

	public:
		ConfigDataBase();
		String GetConfig(const String & name);

		String MakeConfig(const String &name, ProtoType proto, const ProxyConfig & proxy);
		void CleanTmp();

		ProxyConfig GetProxy(const String & name);
		String AddProxy(const String & name);
		void DelProxy(const String & name);
		String GetData(const String & name);
		void SetData(const String & name, const String & value);

		String AddConfig(const String & name);
		void DelConfig(const String & name);
		inline const Map<String, VPNConfig> & GetServers() const { return _conf; }
		inline const Map<String, ProxyConfig> & GetProxys() const {return _proxyes; }
};


class MemServer{
	public:
		typedef bool (*ObserveRequestFunction) (ServerStruct *, MemServer &);
	private:
		ServerStruct * data = nullptr;
		int _memid;
		Map<TypeRequest, List<ObserveRequestFunction>> _responce;

		String * serverlog = nullptr;
		pthread_mutex_t * serverloglock = nullptr;

		bool isExit = false;
		ConfigDataBase _db;


	public:
		inline const ConfigDataBase & GetConfigs() const { return _db; }
		inline ConfigDataBase & GetConfigsWarn() { return _db; }

		inline void log(const String & txt) {
			if (serverlog != nullptr) {
				pthread_mutex_lock(serverloglock);
				(*serverlog) += txt;
				pthread_mutex_unlock(serverloglock);
			}
		}
		inline String poplog() {
			if (serverlog == nullptr)
				return "";
			pthread_mutex_lock(serverloglock);
			String res = (*serverlog);
			(*serverlog) = "";
			pthread_mutex_unlock(serverloglock);
			return res;
		}
		MemServer() {}
		void Create();
		void Listen(TypeRequest alwais = TypeRequest::Allwais);
		void LoadStdFunction();

		inline void SetExit(bool needExit) { isExit = needExit; }

		inline void AddFunction(TypeRequest req, ObserveRequestFunction func) {
			_responce[req].push_back(func);
		}
};
