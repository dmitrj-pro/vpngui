#include "Functions.h"

QString substr(const QString & start, int startpos, int end) {
	QString res ="";
	for (int i = startpos; i < end; i++)
		res += start[i];
	return res;
}


QString GetFileName(const QString & file) {
	QString res = "";
	for (int i =  file.size() -1; i >= 0; i--) {
		if (file[i] == '/' || file[i] == '\\')
			break;
		res = file[i] + res;
	}
	return res;
}

QString GetFileNameWithoutType(const QString & filename) {
	QString file = GetFileName(filename);

	QString res = "";
	int i = file.size() -1;
	for (; i >= 0; i--) {
		if (file[i] == '.')
			break;
	}
	if (i < 0)
		return filename;
	res = substr(file, 0, i);
	return res;
}

QString GetVPNId(const QString & full) {
	QString res = "";
	for (int i = 0 ; i < full.size(); i++) {
		if (full[i] == ':')
			return res;
		res += full[i];
	}
	return "";
}
