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
	Program(const QString& _name, const QString& source_dir, const QString& _work_dir, const QString& _cmd,const QStringList& _args,QObject* parent=nullptr,bool ignoreLogError=false);
	static void InitializeLogs();
	bool Start();
	void Stop();
	bool Restart() { return Start(); }
	bool Fetch();
	bool Deploy(QString& message);
	void Check();
	int PID();
	QString StatusText();
	void ClearError();
signals:
	void signalLogChanged();
	void signalErrorChanged();
protected:
	void AppendLog(const QByteArray& data);
	void LocalLog(const QString & message);
	void OnReadyRead(int channel);
	void OnFinished(int exitCode, QProcess::ExitStatus exitStatus);
public:
	bool enable = true;
	bool deploying = false;
	bool ignore_log_error=false;
	int check_ct=0;
	QString name;
	QString source_dir;
	QString work_dir;
	QString cmd;
	QStringList args;
	QByteArray log_merged;//stdout+stderr,按时间顺序
	QByteArray log_error;//stderr
	bool has_error=false;
	QDateTime start_time;
	QProcess*process = nullptr;
};
