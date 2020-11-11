#pragma once
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSystemTrayIcon>
//#include <QtWidgets/qtablewidget.h>
#include <QDateTime>
#include <QTableWidget>
#include <QProcess>
class Program:public QObject 
{
	Q_OBJECT
public:
	Program(const QString& _name, const QString& _work_dir, const QString& _cmd,const QStringList& _args,QObject* parent=nullptr);
	bool Start();
	bool Restart() { return Start(); }
	void Check();
	int PID();
	QString StatusText();
protected:
	void Stop();
	void LocalLog(const QString & message);
	void OnReadyRead();
	void OnFinished(int exitCode, QProcess::ExitStatus exitStatus);
public:
	QString name;
	QString work_dir;
	QString cmd;
	QStringList args;
	QByteArray log;
	QDateTime start_time;
	QProcess*process = nullptr;
};