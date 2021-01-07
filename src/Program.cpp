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
	log += "--------------------------\n"+
		name +":"+message+ " on " + QDateTime::currentDateTime().toString() + "\n"
		"--------------------------\n";
	emit LogChanged();
}
bool Program::Start()
{
	Stop();
	LocalLog("Start");
	process = new QProcess(this);
	process->setWorkingDirectory(work_dir);
	process->setReadChannelMode(QProcess::ProcessChannelMode::MergedChannels);
	process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
	/*process->setCreateProcessArgumentsModifier(
		[](QProcess::CreateProcessArguments * args){
		args->flags |= CREATE_NEW_CONSOLE;
		args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
	});*/
	connect(process, &QProcess::readyRead, this, &Program::OnReadyRead);
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
	}
	else if(process&&process->state() != QProcess::ProcessState::NotRunning)
		Stop();
//	qDebug() << process->state();
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

void Program::OnReadyRead() 
{
	auto tmp = process->readAll();
	//qDebug() << tmp;
	log += tmp;
	if (log.length() > 10*10000)
		log=log.right(10000);
	emit LogChanged();
}

void Program::OnFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (exitCode != 0 || exitStatus != QProcess::NormalExit)
		LocalLog("Start");
}