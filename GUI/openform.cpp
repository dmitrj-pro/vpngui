#include "openform.h"
#include "ui_openform.h"
#include <QFileDialog>
#include "mainwindow.h"

#include "Functions.h"
#include <Parser/SmartParser.h>
#include <_Driver/Files.h>

using __DP_LIB_NAMESPACE__::SmartParser;
using __DP_LIB_NAMESPACE__::FileExists;
using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::String;

OpenForm::OpenForm(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::OpenForm)
{
	ui->setupUi(this);

	ui->centralWidget->setLayout(ui->verticalLayout);

	connect(ui->buttonOpen, SIGNAL(released()), this, SLOT(OpenConfig()));
	connect(ui->buttonSubmit, SIGNAL(released()), this, SLOT(Confirm()));
	connect(ui->buttonClose, SIGNAL(released()), this, SLOT(Close()));

}

void OpenForm::Close() {
	login = "";
	password = "";
	file ="";
	close();
}

void OpenForm::Confirm() {
	if (file.size() > 0) {
		if (ui->loginLine->isEnabled()) {
			if (ui->loginLine->text().size() == 0 || ui->passwordLine->text().size() == 0) {
				ui->errorText->setEnabled(true);
				ui->errorText->setText("Login or password is not entered");
				return;
			}
			login = ui->loginLine->text();
			password = ui->passwordLine->text();
		}
		this->close();
		MainWindow*  mai = (MainWindow* )parent();
		mai->OpenFileFinal();
	}
}

void OpenForm::OpenConfig() {
	ui->errorText->setEnabled(false);
	QFileDialog * d = new QFileDialog();
	QString res = d->getOpenFileName(this, "Select server", "", "*.txt *.ovpn");
	if (res.size() > 0) {
		ui->passwordLine->setEnabled(false);
		ui->loginLine->setEnabled(false);

		ui->fileLine->setText(res);

		String file = res.toStdString();
		this->file = res;

		static SmartParser log("auth-user-pass*");

		if (FileExists(file)) {
			Ifstream in;
			in.open(file);

			while (!in.eof()) {
				String line;
				getline (in, line);
				if (log.Check(line)) {
					ui->passwordLine->setEnabled(true);
					ui->loginLine->setEnabled(true);
					break;
				}
			}
			in.close();
		}

	}
	delete d;
}

OpenForm::~OpenForm()
{
	delete ui;
}
