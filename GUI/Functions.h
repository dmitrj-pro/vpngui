#pragma once
#include <QString>

QString substr(const QString & start, int startpos, int end);

QString GetFileName(const QString & file);

QString GetFileNameWithoutType(const QString & filename);

QString GetVPNId(const QString & full);
