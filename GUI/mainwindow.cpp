#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sys/types.h>
#include <QTextCursor>
#include <sys/shm.h>
#include "Functions.h"
#include <QFileDialog>
#include "openform.h"
#include "../Core/head.h"
#include <Parser/SmartParser.h>
#include <Converter/Converter.h>

using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::SmartParser;
using __DP_LIB_NAMESPACE__::String;

Client * ClientOne::cl = nullptr;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->centralWidget->setLayout(ui->verticalLayout);

	ui->allServersTab->setLayout(ui->allServersLayout);
	ui->ServerLogTab->setLayout(ui->serverLogLayout);
	ui->statusTab->setLayout(ui->statusTabLayout);
	ui->proxiesTab->setLayout(ui->proxiesTabLayout);
	ui->settingsTab->setLayout(ui->settingsTabLayout);

	serversView = new QStringListModel();
	ui->AllServers->setModel(serversView);
	serversList = nullptr;

	proxyList = nullptr;
	proxyView = new QStringListModel();
	ui->proxiesView->setModel(proxyView);

	ui->proxy_proxyType->addItem("http");
	ui->proxy_proxyType->addItem("socks");

	ui->settings_protocol->addItem("NONE");
	ui->settings_protocol->addItem("TCP");
	ui->settings_protocol->addItem("UDP");

	setWindowIcon(QIcon(":vpn.ico"));

	openform= new OpenForm(this);

	timer = new QTimer();
	timer->setInterval(500);
	connect(timer, SIGNAL(timeout()), this, SLOT(UpdateStatus()));
	connect(ui->buttonConnect, SIGNAL(released()), this, SLOT(ConnectDisconnect()));
	connect(ui->buttonAddServer_2, SIGNAL(released()), this, SLOT(AddServer()));
	connect(ui->buttonUpdateStat, SIGNAL(released()), this, SLOT(UpdateStatistic()));
	connect(ui->buttonDeleteServer, SIGNAL(released()), this, SLOT(DeleteServer()));
	connect(ui->proxy_add, SIGNAL(released()), this, SLOT(AddProxyG()));
	connect(ui->proxy_delete, SIGNAL(released()), this, SLOT(DeleteProxy()));
	timer->start();

	UpdateStatus();
	UpdateServers(true);

	UpdateProxy();
	LoadConfig();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::OpenFileFinal() {
	QString request = GetFileNameWithoutType(openform->GetFile());
	request += ":";
	request += openform->GetFile();
	request += ":";
	if (openform->GetLogin().size() == 0)
		request += "key";
	else {
		request += "loginpassword:";
		request += openform->GetLogin();
		request += ":";
		request += openform->GetPassword();
	}

	std::string tmp = request.toStdString();

	server.AExecVoid(AddConfig, tmp);

	UpdateServers();
}

void MainWindow::AddServer() {
	//OpenForm * form = new OpenForm();
	openform->show();
	//QString config = form->GetFile();
	/*
	QFileDialog * d = new QFileDialog();
	QString res = d->getOpenFileName(this, "Select server", "", "*.txt *.ovpn");
	if (res.size() > 0) {

		QString id = GetFileNameWithoutType(res);
		id += ":";
		id += res + ":";
		//ToDo
		id += "key";

		std::string tmp = id.toStdString();
		pthread_mutex_lock(&lockServer);
		server->request=AddConfig;
		for (int i = 0; i < tmp.size(); i++) {
			server->file[i] = tmp[i];
			server->file[i+1] = 0;
		}

		pthread_mutex_unlock(&server->lock_server);
		pthread_mutex_lock(&server->lock_user);

		pthread_mutex_unlock(&lockServer);
	}
	delete d;
	UpdateServers();*/
}
void MainWindow::AddProxyG() {
	ui->proxy_error->setEnabled(false);
	#define ERROR(X) \
		{\
			ui->proxy_error->setText(X); \
			ui->proxy_error->setEnabled(true); \
			return;\
		}
	ui->proxy_error->setText("");
	if (ui->proxy_name->text().size() == 0)
		ERROR("Name is empty");
	if (ui->proxy_server->text().size() == 0)
		ERROR("Proxy server is empty");
	if (ui->proxy_port->text().size() == 0)
		ERROR("Proxy port is empty");
	{
		bool ok = true;
		int port = ui->proxy_port->text().toInt(&ok);
		if (!ok)
			ERROR("Port is not correct");
		if (port <= 0 || port >= 65000)
			ERROR("Port is number from 1 to 65000");
	}
	QString req = ui->proxy_name->text();
	req += "|";
	//Name|server|port|type
	req += ui->proxy_server->text();
	req += "|";
	req += ui->proxy_port->text();
	req += "|";
	if (ui->proxy_proxyType->currentIndex() == 0)
		req += "http";
	if (ui->proxy_proxyType->currentIndex() == 1)
		req += "socks";
	String tmp = req.toStdString();
	server.AExecVoid(AddProxy, tmp);
	UpdateProxy();

	#undef ERROR
}
void MainWindow::DeleteProxy() {
	if (!ui->proxiesView->currentIndex().isValid())
		return;
	auto tmp = ui->proxiesView->selectionModel()->selectedIndexes();
	if (tmp.size() == 0)
		return;

	QString ids = getIdServer(proxyList->at(tmp.at(0).row()));

	std::string conf = ids.toStdString();
	server.AExecVoid(RmProxy, conf);

	UpdateProxy();
}

QString MainWindow::getIdServer(const QString & path) {
	int i = 0;
	for (; i < path.size(); i++) {
		if (path[i] == ':')
			break;
	}
	return substr(path, 0, i);
}

void MainWindow::DeleteServer() {
	if (!ui->AllServers->currentIndex().isValid())
		return;
	auto tmp = ui->AllServers->selectionModel()->selectedIndexes();
	if (tmp.size() == 0)
		return;

	QString ids = getIdServer(serversList->at(tmp.at(0).row()));

	std::string conf = ids.toStdString();
	server.AExecVoid(RmConfig, conf);

	UpdateServers();

}

void MainWindow::ConnectDisconnect() {
	if (server.Core()->status == Connected || server.Core()->status == Connecting) {
		ui->AllServers->setEnabled(true);
		ui->buttonDeleteServer->setEnabled(true);

		server.AExecVoid(Disconnect);
		return;
	}

	if (server.Core()->status == Disconnected) {
		if (!ui->AllServers->currentIndex().isValid())
			return;

		auto tmp = ui->AllServers->selectionModel()->selectedIndexes();
		if (tmp.size() == 0)
			return;

		ui->AllServers->setEnabled(false);
		ui->buttonDeleteServer->setEnabled(false);

		QString ids = getIdServer(serversList->at(tmp.at(0).row()));
		if (ui->settings_protocol->currentIndex() != 0) {
			if (ui->settings_protocol->currentIndex() == 1) {
				ids += ":";
				ids += "tcp";
			}
			if (ui->settings_protocol->currentIndex() == 2) {
				ids += ":";
				ids += "udp";
			}
			SaveConfig();
		}
		if (ui->settingProxy->currentIndex() != 0) {
			bool needProxy = true;
			if (ui->settings_protocol->currentIndex() != 0) {
				if (ui->settings_protocol->currentIndex() != 1)
					needProxy = false;
			} else {
				ids += ":";
				ids += "tcp";
			}
			if (needProxy) {
				ids += ":";
				ids += ui->settingProxy->currentText();
				SaveConfig();
			}
		}

		std::string conf = ids.toStdString();
		server.AExecVoid(Connect, conf);
		return;
	}
}

QString toQString(const std::string & txt) {
	QString res = "";
	for (int i = 0; i < txt.size(); i++) {
		res += txt[i];
	}
	return res;
}

void MainWindow::UpdateProxy() {
	ui->proxy_delete->setEnabled(false);
	QStringList * list = new QStringList();
	if (proxyList != nullptr)
		delete proxyList;

	proxyList = new QStringList();
	ui->settingProxy->clear();
	ui->settingProxy->addItem("NONE");

	server.lock();

	server.Core()->request = GetProxy;

	std::string line = "";

	while (true) {
		pthread_mutex_unlock(&server.Core()->lock_server);
		pthread_mutex_lock(&server.Core()->lock_user);
		for (int i = 0 ; i < server.Core()->res_size; i++) {
			if (server.Core()->file[i] == '\n') {
				QString l = QString::fromStdString(line);
				QString id = GetVPNId(l);
				if (id.size() > 0) {
					list->push_back(id);
					proxyList->push_back(l);
					ui->settingProxy->addItem(id);
				}
				line = "";
				continue;
			}
			line += server.Core()->file[i];
		}
		if (server.Core()->request != None)
			continue;
		break;
	}
	QString l2 = toQString(line);
	QString id2 = GetVPNId(l2);
	if (id2.size() > 0) {
		list->push_back(id2);
		ui->settingProxy->addItem(id2);
		proxyList->push_back(l2);
	}

	proxyView->setStringList(*list);

	server.unlock();
	if (proxyList->size() > 0)
		ui->proxy_delete->setEnabled(true);
}

void MainWindow::UpdateServers(bool init) {
	QStringList * list = new QStringList();
	if (serversList != nullptr)
		delete serversList;

	serversList = new QStringList();

	server.lock();

	server.Core()->request = GetServers;

	std::string line = "";

	while (true) {
		pthread_mutex_unlock(&server.Core()->lock_server);
		pthread_mutex_lock(&server.Core()->lock_user);
		for (int i = 0 ; i < server.Core()->res_size; i++) {
			if (server.Core()->file[i] == '\n') {
				QString l = QString::fromStdString(line);
				QString id = GetVPNId(l);
				if (id.size() > 0) {
					list->push_back(id);
					serversList->push_back(l);
				}
				line = "";
				continue;
			}
			line += server.Core()->file[i];
		}
		if (server.Core()->request != None)
			continue;
		break;
	}
	QString l2 = toQString(line);
	QString id2 = GetVPNId(l2);
	if (id2.size() > 0) {
		list->push_back(id2);
		serversList->push_back(l2);
	}

	serversView->setStringList(*list);

	if (init) {
		QString in = "";
		for (int i = 0; i < server.Core()->ovpn_size; i++) {
			in += server.Core()->ovpn_file[i];
		}

		for (int i = 0; i < serversList->size(); i++) {
			if (GetVPNId(serversList->at(i)) == in) {
				QModelIndex tmp = serversView->index(i, 0);
				ui->AllServers->setCurrentIndex(tmp);
				if (server.Core()->status == Connecting || server.Core()->status == Connected) {
					ui->AllServers->setEnabled(false);
					ui->buttonDeleteServer->setEnabled(false);
				}
			}
		}

	}

	server.unlock();
}

void MainWindow::UpdateStatus() {
	String logPart = server.AExec(Update_log);


	QString status = "";

	if (server.Core()->status == Disconnected) {
		status = "Disconnected";
		ui->buttonConnect->setText("Connect");
	}
	if (server.Core()->status == Connected) {
		status = "Connected";
		ui->buttonConnect->setText("Disconnect");
	}
	if (server.Core()->status == Connecting) {
		status = "Connecting...";
		ui->buttonConnect->setText("Disconnect");
	}
	ui->Status->setText(status);

	QTextCursor old = ui->ServerLog->textCursor();

	QString log = ui->ServerLog->toPlainText();
	log += QString::fromStdString(logPart);

	ui->ServerLog->setText(log);

	if (old.atEnd())
		ui->ServerLog->moveCursor(QTextCursor::End);
	else
		ui->ServerLog->setTextCursor(old);

	if ((rand()%2) == 1)
		UpdateStatusInformation();
}

QString toHumenFormat(const String & data) {
	double val = parse<double>(data);
	QString res = "";
	int type = 0;
	if (val > 1024) {
		type++;
		val/=1024;
	}
	if (val > 1024) {
		type++;
		val/=1024;
	}
	if (val > 1024) {
		type++;
		val/=1024;
	}
	res = QString::number(val);
	res += " ";


	if (type == 0)
		res = res + "Bytes";
	if (type == 1)
		res = res + "KB";
	if (type == 2)
		res = res + "MB";
	if (type == 3)
		res = res + "GB";
	return res;
}

void MainWindow::UpdateStatusInformation() {
	String ip = runRead("ifconfig");
	static SmartParser tap ("tap*");
	static SmartParser tun ("tun*");
	bool enabled = false;
	static SmartParser ipLocal2("*inet addr:${ip:trim<string>}P-*");
	static SmartParser ipLocal("*inet${ip:trim<string>}netmask*");
	static SmartParser sendPac("*RX*bytes${data:trim<string>}(*");
	static SmartParser sendPac2("*RX*bytes:${data:trim<string>}(*");
	static SmartParser recPac2("*TX*bytes:${data:trim<string>}(*");
	static SmartParser recPac("*TX*bytes${data:trim<string>}(*");

	__DP_LIB_NAMESPACE__::IStrStream in;
	in.str(ip);
	while (!in.eof()) {
		String line;
		getline(in, line);
		if (enabled) {
			if (ipLocal.Check(line)) {
				ui->labelLocalIP->setText(QString::fromStdString(ipLocal.Get("ip")));
			}
			if (sendPac.Check(line))
				ui->labelReceavedTraffic->setText(toHumenFormat(sendPac.Get("data")));
			if (recPac.Check(line)) {
				enabled = false;
				ui->labelSendedTraffic->setText(toHumenFormat(recPac.Get("data")));
			}

			if (ipLocal2.Check(line)) {
				ui->labelLocalIP->setText(QString::fromStdString(ipLocal2.Get("ip")));
			}
			if (sendPac2.Check(line))
				ui->labelReceavedTraffic->setText(toHumenFormat(sendPac2.Get("data")));
			if (recPac2.Check(line)) {
				ui->labelSendedTraffic->setText(toHumenFormat(recPac2.Get("data")));
				enabled = false;
			}
		} else {
			if (tap.Check(line) || tun.Check(line)) {
				enabled = true;
			}
		}
	}

	currentSleepIndex++;
	if (currentSleepIndex>countSleep) {
		ui->labelGlobalIP->setText(QString::fromStdString(runRead("wget -qO- ipecho.net/ip")));
		currentSleepIndex = 0;
	}
}

void MainWindow::UpdateStatistic() {
	currentSleepIndex = countSleep;
	UpdateStatusInformation();
}

void MainWindow::LoadConfig() {
	String data = server.AExec(GetData, "proto");
	if (data == "none")
		ui->settings_protocol->setCurrentIndex(0);
	if (data == "tcp")
		ui->settings_protocol->setCurrentIndex(1);
	if (data == "udp")
		ui->settings_protocol->setCurrentIndex(2);

	data = server.AExec(GetData, "proxy");
	int parsed = parse<int>(data);
	ui->settingProxy->setCurrentIndex(parsed);
}

void MainWindow::SaveConfig() {
	SmartParser val("${name}:${value}");
	val.Set("name", "proto");
	if (ui->settings_protocol->currentIndex() == 0)
		val.Set("value", "none");
	if (ui->settings_protocol->currentIndex() == 1)
		val.Set("value", "tcp");
	if (ui->settings_protocol->currentIndex() == 2)
		val.Set("value", "udp");
	server.AExecVoid(SaveData, val.ToString());

	val.Set("name", "proxy");
	val.Set("value", __DP_LIB_NAMESPACE__::toString(ui->settingProxy->currentIndex()));
	server.AExecVoid(SaveData, val.ToString());
}
