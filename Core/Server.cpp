#include "Server.h"
#include "INCLUDE.h"
#include <sys/shm.h>
#include <mutex>
#include "head.h"
#include <thread>
#include <string.h>
#include <sys/wait.h>
#include <Types/Exception.h>
#include <Parser/SmartParser.h>
#include <Parser/Setting.h>
#include <Crypt/Crypt.h>
#include <Converter/Converter.h>
#include <_Driver/Files.h>

using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::FileExists;
using __DP_LIB_NAMESPACE__::Ofstream;
using __DP_LIB_NAMESPACE__::startWithN;
using __DP_LIB_NAMESPACE__::Setting;
using __DP_LIB_NAMESPACE__::SmartParser;
using __DP_LIB_NAMESPACE__::Crypt;
using __DP_LIB_NAMESPACE__::IStrStream;
using __DP_LIB_NAMESPACE__::OStrStream;
using __DP_LIB_NAMESPACE__::Vector;
using __DP_LIB_NAMESPACE__::split;
using __DP_LIB_NAMESPACE__::ConteinsKey;

void init_memory(char * c, int size) {
	for (int i = 0 ; i < size; i++ )
		c[i] = 0;
}

/// ConfigDataBase

String AllRead(const String & fileName) {
	Ifstream in;
	in.open(fileName);
	if (in.fail())
		throw EXCEPTION("Fail to open file " + fileName);

	String res;
	while (!in.eof() ) {
		String line;
		getline(in, line);
		res += line + "\n";
	}
	in.close();
	return res;
}

void CP(const String & before, const String & after) {
	Ifstream in;
	in.open(before);
	if (in.fail()) {
		LOG2("Fail to open file %s", before.c_str());
		return;
	}
	Ofstream out;
	out.open(after);
	if (out.fail()) {
		LOG2("Fail copy file to %s", after.c_str());
		return;
	}

	while (!in.eof()) {
		String line;
		getline(in, line);
		out << line << "\n";
	}
	out.close();
	LOG2("File %s copyed", after.c_str());
}

ConfigDataBase::ConfigDataBase() {
	Load();
}

void ConfigDataBase::Save() {
	LOG("Save config");
	String conf="";
	for (auto it = _conf.begin(); it != _conf.end(); it++) {
		conf += toStringConf(*it);
		conf += ";";
	}

	String proxy = "";
	for (auto it = _proxyes.begin(); it != _proxyes.end(); it++) {
		proxy += toStringProxy(*it);
		proxy += ";";
	}

	Setting set;
	set.add("configs", conf);
	set.add("proxy", proxy);

	for (auto it = _save_data.cbegin(); it != _save_data.cend(); it++) {
		String key = "user.";
		key += it->first;
		set.add(key, it->second);
	}

	OStrStream out;
	out << set;
	String data = "";
	#ifdef ENCCONF
		Crypt cr (ENC_PASSWORD, "SCH5");
		data = cr.Enc(out.str());
	#else
		data = out.str();
	#endif

	Ofstream ofile;
	ofile.open(ConfigDB);
	if (ofile.fail()) {
		LOG2("Fail to save config file %s", ConfigDB );
		return;
	}
	ofile << data;
	ofile.close();
	LOG("Config saved");
}

void LoadAllUserData(Map<String, String> &data, Setting & set, const String & key, const String & add) {
	String k = add;
	if (add.size() != 0)
		k += ".";

	List<String> all = set.getKeys<List<String>>(key);
	for (auto it = all.begin(); it != all.end(); it ++ ) {
		String k2 = k;
		k2 += *it;
		String hk = key;
		hk += ".";
		hk += *it;

		LOG2("Add data %s", k2.c_str());
		LOG2("with value %s", set.get(hk).c_str());
		data[k2] = set.get(hk);
	}

	List<String> dir = set.getFolders<List<String>>(key);
	for (auto it = dir.begin(); it != dir.end(); it ++ ) {
		String k2 = key;
		k2 += ".";
		k2 += *it;
		String a = add;
		if (add.size() != 0)
			a += ".";
		a += *it;
		LoadAllUserData(data, set, k2, a);
	}
}

void ConfigDataBase::Load() {
	LOG("Try load config");
	String conf="";
	try{
		conf = AllRead(ConfigDB);
	} catch(...) {
		LOG2("File %s is not exists. Break", ConfigDB);
		return;
	}
	String data = "";
	#ifdef ENCCONF
		if (startWithN(conf, "SCH")) {
			Crypt cr (ENC_PASSWORD, "SCH5");
			data = cr.Dec(conf);
			if (data.size() < 3)
				throw EXCEPTION("Password is not correct");
		} else
	#endif
			data = conf;
	IStrStream stream;
	stream.str(data);
	Setting set;
	stream * set;

	String configs = set.get("configs");
	Vector<String> config = split(configs, ';');
	for (auto it = config.begin(); it != config.end(); it++) {
		Pair<String, ConfigDataBase::VPNConfig> pair = parseString(*it);
		if (pair.first.size() > 0) {
			_conf[pair.first] = pair.second;
		}
	}

	String proxyes = set.get("proxy");
	Vector<String> proxy = split(proxyes, ';');
	for (auto it = proxy.begin(); it != proxy.end(); it++) {
		Pair<String, ProxyConfig> pair = parseStringProxy(*it);
		if (pair.first.size() > 0) {
			_proxyes[pair.first] = pair.second;
		}
	}

	LoadAllUserData(_save_data, set, "user", "");

	LOG2("Loaded %i configs", config.size());
}

String ConfigDataBase::GetConfig(const String & name) {
	if (ConteinsKey(_conf, name)) {
		return _conf[name].file;
	} else
		return "";
}

ConfigDataBase::ProxyConfig ConfigDataBase::GetProxy(const String & name) {
	if (ConteinsKey(_proxyes, name)) {
		return _proxyes[name];
	} else
		return ProxyConfig();
}

bool CheckSymbol(char c) {
	if ((c == '|') || (c == ';') || (c == ':') || (c == '#') || (c == '!') || (c == '&') || (c == '@') || (c == '$') || (c == '%') || (c == '^') || (c == '*') || (c == '(') || (c == ')') || (c == '\n'))
		return false;
	return true;
}

// IN: Name:File:Type:?Login:?Password
Pair<String, ConfigDataBase::VPNConfig> ConfigDataBase::parseString(const String & conf) {
	LOG2("Try parse %s", conf.c_str());
	Pair<String, ConfigDataBase::VPNConfig> res;
	int stat = 0;
	String type = "";
	for (int i = 0; i < conf.size(); i++) {
		if (conf[i] == ':') {
			if (stat == 2) {
				res.second.type = parseType(type);
			}
			stat++;
			continue;
		}
		if (stat == 0) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::VPNConfig>();
			res.first += conf[i];
		}
		if (stat == 1) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::VPNConfig>();
			res.second.file += conf[i];
		}
		if (stat == 2) {
			type += conf[i];
		}
		if (stat == 3) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::VPNConfig>();
			res.second.login += conf[i];
		}
		if (stat == 4) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::VPNConfig>();
			res.second.password += conf[i];
		}
	}
	if (stat < 2)
		return Pair<String, ConfigDataBase::VPNConfig>();

	if (stat == 2) {
		res.second.type = parseType(type);
	}
	return res;
}

String ConfigDataBase::toStringConf(const Pair<String, VPNConfig> & conf) {
	String res;
	res += conf.first + ":";
	res += conf.second.file + ":";
	res += toStringType(conf.second.type);
	if (conf.second.type == VPNConfig::ConfigType::LoginPassword) {
		res += ":" + conf.second.login + ":";
		res += conf.second.password;
	}
	return res;
}

//Name|server|port|type
Pair<String, ConfigDataBase::ProxyConfig> ConfigDataBase::parseStringProxy(const String & conf) {
	LOG2("Try parse %s", conf.c_str());
	Pair<String, ConfigDataBase::ProxyConfig> res;
	int stat = 0;
	String type = "";
	String port = "";
	for (int i = 0; i < conf.size(); i++) {
		if (conf[i] == '|') {
			if (stat == 2)
				res.second.port = __DP_LIB_NAMESPACE__::parse<UInt>(port);
			if (stat == 3)
				res.second.type = parseStringProxyType(type);
			stat++;
			continue;
		}
		if (stat == 0) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::ProxyConfig>();
			res.first += conf[i];
		}
		if (stat == 1) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::ProxyConfig>();
			res.second.server += conf[i];
		}
		if (stat == 2) {
			if (!CheckSymbol(conf[i]))
				return Pair<String, ConfigDataBase::ProxyConfig>();
			port += conf[i];
		}
		if (stat == 3) {
			type += conf[i];
		}
	}
	if (stat < 3)
		return Pair<String, ConfigDataBase::ProxyConfig>();

	if (stat == 3) {
		res.second.type = parseStringProxyType(type);
	}
	return res;
}
String ConfigDataBase::toStringProxy(const Pair<String, ProxyConfig> & conf) {
	String res;
	res += conf.first + "|";
	res += conf.second.server + "|";
	res += __DP_LIB_NAMESPACE__::toString(conf.second.port);
	res += "|";
	res += toStringType(conf.second.type);
	return res;
}

String GetFileName(const String & file) {
	String res = "";
	for (int i =  file.size() -1; i >= 0; i--) {
		if (file[i] == '/' || file[i] == '\\')
			break;
		res = file[i] + res;
	}
	return res;
}

String GetFilePath(const String & file) {
	String res = "";
	int i = file.size() -1;
	for (; i >= 0; i--) {
		if (file[i] == '/' || file[i] == '\\')
			break;
	}
	res = file.substr(0, i);
	return res;
}

String ConfigDataBase::AddProxy(const String & name) {
	LOG2("Try add proxy %s", name.c_str());
	Pair<String, ConfigDataBase::ProxyConfig> pair = parseStringProxy(name);

	if (ConteinsKey(_proxyes, pair.first)) {
		LOG("Proxy is added.");
		return pair.first;
	}

	_proxyes[pair.first] = pair.second;

	Save();

	return pair.first;
}

String ConfigDataBase::AddConfig(const String & name) {
	LOG2("Try add config %s", name.c_str());
	Pair<String, ConfigDataBase::VPNConfig> pair = parseString(name);

	if (ConteinsKey(_conf, pair.first)) {
		LOG("Config is added.");
		return pair.first;
	}
	if (pair.first.size() > 0) {
		if (FileExists(pair.second.file)) {
			Ifstream in;
			in.open(pair.second.file);
			if (in.fail()) {
				LOG("Fail to open config file");
				return "";
			}
			Ofstream out;


			#ifdef ENCCONF
				String newFile = VPN_CONFIGS_PATH;
				newFile += "/";
				newFile += EncryptName(GetFileName(pair.second.file), _crypt);
				out.open(newFile, std::ios::binary);
			#else
				String newFile = VPN_CONFIGS_PATH;
				newFile += "/";
				newFile += GetFileName(pair.second.file);

				out.open(newFile);
			#endif
			LOG2("Try save %s", newFile.c_str());

			if (out.fail()) {
				LOG("Fail to open new config file to write");
				in.close();
				return "";
			}

			while (!in.eof()) {
				String line;
				getline (in, line);

				static SmartParser ca("ca ${file:trim<string>}");
				static SmartParser cert("cert ${file:trim<string>}");
				static SmartParser key("key ${file:trim<string>}");

				static SmartParser log("auth-user-pass*");

				if (ca.Check(line)) {
					LOG2("Detected ca in line '%s'", line.c_str());

					String file = VPN_CONFIGS_PATH;
					file += "/";
					file += ca.Get("file");



					LOG2("New ca path '%s'", file.c_str());
					if (!FileExists(file)) {
						String before = GetFilePath(pair.second.file);
						before += "/";
						before += ca.Get("file");
						CP(before, file);
					}

					ca.Set("file", file);
					line = ca.ToString();
				}

				if (cert.Check(line)) {
					LOG2("Detected cert in line '%s'", line.c_str());

					String file = VPN_CONFIGS_PATH;
					file += "/";
					file += cert.Get("file");



					LOG2("New ca path '%s'", file.c_str());
					if (!FileExists(file)) {
						String before = GetFilePath(pair.second.file);
						before += "/";
						before += cert.Get("file");
						CP(before, file);
					}

					cert.Set("file", file);
					line = cert.ToString();
				}

				if (key.Check(line)) {
					LOG2("Detected key in line '%s'", line.c_str());

					String file = VPN_CONFIGS_PATH;
					file += "/";
					file += key.Get("file");



					LOG2("New ca path '%s'", file.c_str());
					if (!FileExists(file)) {
						String before = GetFilePath(pair.second.file);
						before += "/";
						before += key.Get("file");
						CP(before, file);
					}

					key.Set("file", file);
					line = key.ToString();
				}

				if (log.Check(line)) {
					LOG2("Detected auth login in line '%s'", line.c_str());

					line = "auth-user-pass ";
					line += VPN_CONFIGS_PATH;
					line += "/";
					line += TMP_LOG_FILE;
				}
				#ifdef ENCCONF
					String tmp_enc = line;
					tmp_enc += "\n";
					EncryptString(out, tmp_enc, _crypt);
				#else
					out << line << "\n";
				#endif
			}

			pair.second.file = newFile;

			out.close();

			_conf[pair.first] = pair.second;
			Save();
		}
	}

	return pair.first;
}

void ConfigDataBase::DelProxy(const String & name) {
	if (!ConteinsKey(_proxyes, name))
		return;

	_proxyes.erase(_proxyes.find(name));
	Save();
}

void ConfigDataBase::DelConfig(const String & name){
	if (!ConteinsKey(_conf, name))
		return;
	VPNConfig x = _conf[name];

	Ofstream out;
	out.open(x.file);
	out << " \n";
	out.close();

	_conf.erase(_conf.find(name));
	Save();
}

#ifdef ENCCONF
	String getLine(std::ifstream & in, ENC::SCH1 & crypt) {
#else
	String getLine(std::ifstream & in) {
#endif
	#ifdef ENCCONF
		crypt.Reset();
		String line = "";
		__DP_LIB_NAMESPACE__::Char c;
		while (in.get(c)) {
			__DP_LIB_NAMESPACE__::Char k = (__DP_LIB_NAMESPACE__::Char) ( c ^ crypt.GetValue(256));
			if (k == '\n') {
				return line;
			}
			line += k;
		}
		return line;
	#else
		String line;
		getline(in, line);
		return line;
	#endif
}

String ConfigDataBase::GetData(const String & name) {
	if (!ConteinsKey(_save_data, name)) {
		LOG2("Unknow parametr %s", name.c_str());
		return "NONE";
	}
	return _save_data[name];
}

void ConfigDataBase::SetData(const String & name, const String & value) {
	_save_data[name] = value;
	Save();
}

String ConfigDataBase::MakeConfig(const String &name, ProtoType proto, const ProxyConfig & proxy) {
	if (!ConteinsKey(_conf, name))
		return "";

	VPNConfig conf = _conf[name];


	Ifstream in;
	#ifdef ENCCONF
		in.open(conf.file, std::ios::binary);
	#else
		in.open(conf.file);
	#endif

	Ofstream out;
	String newFile = VPN_CONFIGS_PATH;
	newFile += "/";
	newFile += TMP_CONF_FILE;
	tmp_file.push_back(newFile);
	out.open(newFile);

	static SmartParser remoteParser("remote ${ip} ${port} ${proto}");
	static SmartParser protoParser("proto ${proto}");
	bool protoInit = false;

	while (!in.eof()) {
		#ifdef ENCCONF
			String line = getLine(in, _crypt);
		#else
			String line = getLine(in);
		#endif
		if (proto != ProtoType::None) {
			if (remoteParser.Check(line)) {
				if (protoInit)
					continue;
				protoInit = true;
				remoteParser.Set("proto", toStringProto(proto));
				out << remoteParser.ToString() << "\n";
				if (proto == ProtoType::TCP && proxy.server.size() > 0) {
					out << toStringType(proxy.type) << "-proxy " << proxy.server << " " << proxy.port << "\n";
				}
				continue;
			}
			if (protoParser.Check(line)) {
				if (protoInit)
					continue;
				protoInit = true;
				protoParser.Set("proto", toStringProto(proto));
				out << protoParser.ToString() << "\n";
				if (proto == ProtoType::TCP && proxy.server.size() > 0) {
					out << toStringType(proxy.type) << "-proxy " << proxy.server << " " << proxy.port << "\n";
				}
				continue;
			}
		}

		out << line << "\n";
	}

	if (conf.type == VPNConfig::ConfigType::LoginPassword) {
		LOG("Generate login password file");

		String file = VPN_CONFIGS_PATH;
		file += "/";
		file += TMP_LOG_FILE;

		Ofstream out;
		out.open(file);

		out << conf.login << "\n" << conf.password << "\n";
		out.close();

		tmp_file.push_back(file);
	}

	String file ="";
	for (int i = 0; i < newFile.size(); i++) {
		if (newFile[i] == ' ') {
			file += "\\ ";
		} else {
			file += newFile[i];
		}
	}
	return file;
}
void ConfigDataBase::CleanTmp() {
	for (auto it = tmp_file.begin(); it != tmp_file.end(); it++){
		Ofstream out;
		out.open(*it);
		out << " \n";
		out.close();
	}
	tmp_file.clear();
}

void MemServer::Create() {
	int _memid = shmget(PortMemoryServer, sizeof(ServerStruct), IPC_CREAT | 0777);
	if (_memid < 0) {
		LOG("Error create server")
		return;
	}
	data = (ServerStruct * ) shmat(_memid, NULL, 0);
	data->request = None;
	data->status = Disconnected;
	data->res_size = 0;
	init_memory(data->file, ADD_SIZE);
	init_memory(data->ovpn_file, ADD_SET_SIZE);

	pthread_mutexattr_t atr_user;
	pthread_mutexattr_init(&atr_user);
	pthread_mutexattr_setpshared(&atr_user, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&data->lock_user, &atr_user);

	pthread_mutexattr_t atr_server;
	pthread_mutexattr_init(&atr_server);
	pthread_mutexattr_setpshared(&atr_server, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&data->lock_server, &atr_server);

	pthread_mutexattr_t atr_used;
	pthread_mutexattr_init(&atr_used);
	pthread_mutexattr_setpshared(&atr_used, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&data->used, &atr_used);

	pthread_mutex_lock(&data->lock_server);
	pthread_mutex_lock(&data->lock_user);

	std::thread * listen = nullptr;

	serverlog = new String();
	serverloglock = new pthread_mutex_t;
	*serverloglock = PTHREAD_MUTEX_INITIALIZER;

	LOG("Server inited\n");
	log("Server inited\n");
}

void MemServer::Listen(TypeRequest alwais) {
	while (true) {
		pthread_mutex_lock(&data->lock_server);

		List<ObserveRequestFunction> & all = _responce[alwais];

		for (auto it = all.begin(); it != all.end(); it++ ) {
			if ((*it)(data, (*this)))
				break;
		}


		List<ObserveRequestFunction> & func = _responce[data->request];

		for (auto it = func.begin(); it != func.end(); it++ ) {
			if ((*it)(data, (*this)))
				break;
		}

		if (isExit) {
			pthread_mutex_unlock(&data->lock_user);
			break;
		}

		data->request = None;
		pthread_mutex_unlock(&data->lock_user);
	}
	shmctl(_memid, IPC_RMID, NULL);
	data = nullptr;
	delete serverlog;
	delete serverloglock;
}



/// Functions
bool _exit (ServerStruct *, MemServer & srv) {
	srv.SetExit(true);
	srv.log("Close server\n");
	LOG("Quit server\n");
	return true;
}

bool _update_log(ServerStruct * str, MemServer & srv) {
	String log = srv.poplog();
	int max = log.size() > ADD_SIZE ? ADD_SIZE : log.size();
	int i = 0;
	for (; i < max; i++)
		str->file[i] = log[i];
	str->file[i] = 0;
	str->res_size = max;
	return true;
}

bool _add_config(ServerStruct * data, MemServer & srv) {
	srv.log("Add new config\n");
	LOG("Add new config\n");
	String conf = data->file;
	ConfigDataBase & bd = srv.GetConfigsWarn();
	String newName = bd.AddConfig(conf);
	int i = 0;
	for (; i < newName.size() ; i++ )
		data->file[i] = newName[i];
	data->file[i] = 0;
	data->res_size = newName.size();
	srv.log("New config " + newName + " added");
	LOG2("New config %s added", newName.c_str());
	return true;
}

bool _rm_config(ServerStruct * data, MemServer & srv) {
	String conf = data->file;
	ConfigDataBase & bd = srv.GetConfigsWarn();
	bd.DelConfig(conf);
	srv.log("Config " + conf + " deleted\n");
	LOG2("Config %s deleted\n", conf.c_str());
	return true;
}

std::thread * __listenVPNConnecter = nullptr;
int __status_conn = 0;

void connect_and_listen(String file, MemServer * server) {
	String cmd = "openvpn ";
	cmd += file;
	int out;
	pid_t pid = io_run(cmd.c_str(), nullptr, &out);
	char buffer[buf_size];
	init_memory(buffer, buf_size);

	int readed;

	while (readed = read(out, buffer, buf_size)) {
		LOG2("VPN res: %s\n", buffer)

		buffer[readed] = 0;
		if (readed > 0) {
			server->log(buffer);
			if (strstr(buffer, "Initialization Sequence Completed"))
				__status_conn = 1;
		}
		sleep(1);
	}

	LOG("VPN finished")

	int status;
	waitpid(pid, &status, 0);
	__status_conn = -1;
}

bool _connect(ServerStruct * data, MemServer & srv) {
	if (__listenVPNConnecter != nullptr) {
		LOG("CRITICAL: VPN connected\n");
		srv.log("CRITICAL: VPN connected\n");
	} else {
		ConfigDataBase & bd = srv.GetConfigsWarn();
		String file = "";
		String proxy = "";
		String type = "";
		{
			String txt = data->file;
			int stat =0;
			for (int i = 0; i < txt.size(); i++) {
				if (txt[i] == ':') {
					stat ++;
					continue;
				}
				if (stat == 0)
					file += txt[i];
				if (stat == 1)
					type += txt[i];
				if (stat == 2)
					proxy += txt[i];
			}
		}

		strcpy(data->ovpn_file, file.c_str());
		data->ovpn_file[file.size()] = 0;
		data->ovpn_size = file.size();

		ConfigDataBase::ProxyConfig proxyConf = bd.GetProxy(proxy);
		LOG2("Start vpn type %s", type.c_str());

		String conf = bd.MakeConfig(file, ConfigDataBase::parseProto(type), proxyConf);

		__listenVPNConnecter = new std::thread(connect_and_listen, conf, &srv);

		data->status = Connecting;
	}
	return true;
}

bool _disconnect (ServerStruct * data, MemServer & srv) {
	exec("killall openvpn");
	srv.log("Disconnect\n");
	LOG("Disconnect\n");

	data->status = Disconnected;
	if (__listenVPNConnecter != nullptr) {
		__listenVPNConnecter->join();
		delete __listenVPNConnecter;
		__listenVPNConnecter = nullptr;
	}
	return true;
}

bool _cleanDataBase(ServerStruct * data, MemServer & srv) {
	if (data->status != Connecting) {
		ConfigDataBase & bd = srv.GetConfigsWarn();
		bd.CleanTmp();
	}
	return false;
}

bool _update_status(ServerStruct * data, MemServer & srv) {
	if (__status_conn < 0) {
		if (__listenVPNConnecter != nullptr) {
			__listenVPNConnecter->join();
			delete __listenVPNConnecter;
			__listenVPNConnecter = nullptr;
		}
		__status_conn = 0;
		data->status = Disconnected;
	}

	if (__status_conn == 1)
		data->status = Connected;


	return false;
}

bool _get_server_list(ServerStruct * data, MemServer & srv) {
	const ConfigDataBase & bd = srv.GetConfigs();
	const Map<String, ConfigDataBase::VPNConfig> & servers = bd.GetServers();
	String res= "";
	for (auto it = servers.cbegin(); it != servers.cend(); it++) {
		res += it->first+":";
		res += it->second.file;
		res += "\n";
	}
	int i = 0;
	LOG2("All size %i", res.size());
	while ((i+1) < res.size()) {
		int max = (res.size() - i) > ADD_SIZE ? ADD_SIZE - 1 : res.size() - i;
		LOG2("Pos %i", max);
		int j =0;
		for (; j < max; j++ ) {
			data->file[j] = res[j+i];
		}
		data->res_size = max;
		i+= max;
		if (i < res.size()) {
			pthread_mutex_unlock(&data->lock_user);
			pthread_mutex_unlock(&data->lock_server);
		}
	}
	LOG("_get_server_list quit");
}

bool _add_proxy(ServerStruct * data, MemServer & srv) {
	srv.log("Add new proxy\n");
	LOG("Add new proxy\n");
	String conf = data->file;
	ConfigDataBase & bd = srv.GetConfigsWarn();
	String newName = bd.AddProxy(conf);
	int i = 0;
	for (; i < newName.size() ; i++ )
		data->file[i] = newName[i];
	data->file[i] = 0;
	data->res_size = newName.size();
	srv.log("New proxy " + newName + " added");
	LOG2("New proxy %s added", newName.c_str());
	return true;
}

bool _get_proxy_list(ServerStruct * data, MemServer & srv) {
	const ConfigDataBase & bd = srv.GetConfigs();
	const Map<String, ConfigDataBase::ProxyConfig> & proxy = bd.GetProxys();
	String res= "";
	for (auto it = proxy.cbegin(); it != proxy.cend(); it++) {
		res += it->first+":";
		res += it->second.server;
		res += "\n";
	}
	int i = 0;
	LOG2("All size %i", res.size());
	while ((i+1) < res.size()) {
		int max = (res.size() - i) > ADD_SIZE ? ADD_SIZE - 1 : res.size() - i;
		LOG2("Pos %i", max);
		int j =0;
		for (; j < max; j++ ) {
			data->file[j] = res[j+i];
		}
		data->res_size = max;
		i+= max;
		if (i < res.size()) {
			pthread_mutex_unlock(&data->lock_user);
			pthread_mutex_unlock(&data->lock_server);
		}
	}
	LOG("_get_server_list quit");
}

bool __rm_proxy(ServerStruct * data, MemServer & srv) {
	String conf = data->file;
	ConfigDataBase & bd = srv.GetConfigsWarn();
	bd.DelProxy(conf);
	srv.log("Proxy " + conf + " deleted\n");
	LOG2("Proxy %s deleted\n", conf.c_str());
	return true;
}

bool __save_data(ServerStruct * data, MemServer & srv) {
	String conf = data->file;
	int i = 0;
	for (; i < conf.size(); i++)
		if (conf[i] == ':')
			break;
	String name = conf.substr(0, i);
	String val = conf.substr(i+1);
	ConfigDataBase & bd = srv.GetConfigsWarn();
	bd.SetData(name, val);
	srv.log("Save new information");
	LOG2("Save data %s", conf.c_str());
	return true;
}

bool __get_data(ServerStruct * data, MemServer & srv) {
	ConfigDataBase & bd = srv.GetConfigsWarn();
	String conf = data->file;
	String val = bd.GetData(conf);
	for (int i = 0; i < val.size(); i++) {
		data->file[i] = val[i];
		data->file[i+1] = 0;
	}
	data->res_size = val.size();
	LOG2("GetData %s", conf.c_str());
	return true;
}

void MemServer::LoadStdFunction() {
	//SaveData, GetData}
	AddFunction(Disconnect, _disconnect);
	AddFunction(Exit, _exit);
	AddFunction(Update_log, _update_log);
	AddFunction(AddConfig, _add_config);
	AddFunction(RmConfig, _rm_config);
	AddFunction(Connect, _connect);
	AddFunction(GetServers, _get_server_list);
	AddFunction(Allwais, _cleanDataBase);
	AddFunction(Allwais, _update_status);
	AddFunction(AddProxy, _add_proxy);
	AddFunction(GetProxy, _get_proxy_list);
	AddFunction(RmProxy, __rm_proxy);
	AddFunction(SaveData, __save_data);
	AddFunction(GetData, __get_data);
}
