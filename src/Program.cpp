#include "Program.h"
#include <Windows.h>
#include <QMenu>
#include <QTimer>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDebug>
Program::Program(const QString& _name, const QString& _work_dir, const QString& _cmd, const QStringList& _args, QObject* parent) :
	name(_name),work_dir(_work_dir),cmd(_cmd),args(_args),QObject(parent)
{

}
void Program::Stop()
{
	if (process)
	{
		process->close();
		process->waitForFinished();
		delete process;
		process = nullptr;
		LocalLog("Terminate");
	}
}
void Program::LocalLog(const QString& message) {
	auto text = ("<font color=\"#0000FF\">" +
		name +":"+message+ " on " + QDateTime::currentDateTime().toString() + "</font>\r\n").toLocal8Bit();
	log_merged += text;
	log_error += text;
	emit signalLogChanged();
}
bool Program::Start()
{
	Stop();
	LocalLog("Start");
	process = new QProcess(this);
	process->setWorkingDirectory(work_dir);
	process->setReadChannelMode(QProcess::ProcessChannelMode::SeparateChannels);
	process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
	/*process->setCreateProcessArgumentsModifier(
		[](QProcess::CreateProcessArguments * args){
		args->flags |= CREATE_NEW_CONSOLE;
		args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
	});*/
	connect(process, &QProcess::channelReadyRead, this, &Program::OnReadyRead);
	connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),this,&Program::OnFinished);
	process->start(cmd,args,QIODevice::ReadOnly);	
	start_time = QDateTime::currentDateTime();
	return process->waitForStarted();
}

void Program::Check()
{
	if (enable)
	{
		if ((!process)||process->state() == QProcess::ProcessState::NotRunning )
			Restart();
		else
		{
			check_ct++;
			if (check_ct % (1200*12)==0)//1200*12*3000ms=12Сʱ
			{
				LocalLog("Check");
				check_ct = 0;
			}
		}
	}
	else if(process&&process->state() != QProcess::ProcessState::NotRunning)
		Stop();
}

int Program::PID()
{
	if (process)
		return process->processId();
	return -1;
}

QString Program::StatusText()
{
	if (process)
	{
		if (process->state() == QProcess::ProcessState::Running)
			return "Running";
		else if (process->state() == QProcess::ProcessState::NotRunning)
			return "Terminating";
		else if (process->state() == QProcess::ProcessState::Starting)
			return "Starting";
	}
	return QString("None");
}

void Program::ClearError()
{
	has_error = false;
	signalErrorChanged();
}

void Program::OnReadyRead(int channel)
{
	QByteArray tmp;
	if (channel == QProcess::StandardOutput)
		tmp = process->readAllStandardOutput();
	else
	{
		tmp = process->readAllStandardError();
		log_error += tmp;
		tmp = "<font color=\"#FF0000\">" + tmp + "</font>";
		has_error = true;
	}
	log_merged += tmp;
	if (log_merged.length() > 10*10000)
		log_merged=log_merged.right(10000);
	if (log_error.length() > 10 * 10000)
		log_error = log_error.right(10000);
	emit signalLogChanged();
}

void Program::OnFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (exitCode != 0 || exitStatus != QProcess::NormalExit)
		LocalLog("Start");
}