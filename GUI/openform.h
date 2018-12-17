#ifndef OPENFORM_H
#define OPENFORM_H

#include <QMainWindow>

namespace Ui {
	class OpenForm;
}

class OpenForm : public QMainWindow
{
	Q_OBJECT

	public:
		explicit OpenForm(QWidget *parent = 0);
		~OpenForm();

	private:
		Ui::OpenForm *ui;

	public slots:
		void OpenConfig();
		void Confirm();
		void Close();

	private:
		QString file;
		QString login;
		QString password;
	public:
		inline QString GetLogin() { return login; }
		inline QString GetPassword() { return password; }
		inline QString GetFile() { return file; }
};

#endif // OPENFORM_H
