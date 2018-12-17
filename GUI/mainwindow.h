#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "../Core/INCLUDE.h"
#include <QStringListModel>

namespace Ui {
class MainWindow;
}

class OpenForm;

class MainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = 0);
		~MainWindow();
		void OpenFileFinal();


	private:
		Ui::MainWindow *ui;
		QTimer * timer;
		QStringListModel * serversView;
		QStringList * serversList;

		QStringListModel * proxyView;
		QStringList * proxyList;

		QString getIdServer(const QString & path);

		void UpdateServers(bool init = false);
		void UpdateProxy();
		int countSleep=100;
		int currentSleepIndex = 0;
		void UpdateStatusInformation();
		void LoadConfig();
		void SaveConfig();

		OpenForm * openform;
		ClientOne server;

	public slots:
		void UpdateStatus();
		void ConnectDisconnect();
		void AddServer();
		void UpdateStatistic();
		void DeleteServer();

		void AddProxyG();
		void DeleteProxy();
};

#endif // MAINWINDOW_H
