#include "Program.h"
#include <Windows.h>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
Program::Program(const QString& _name, const QString& _source_dir, const QString& root_dir, const QString& _cmd, const QStringList& _args, QObject* parent,bool ignoreLogError) :
	name(_name),work_dir(root_dir+_name),args(_args),ignore_log_error(ignoreLogError), source_dir(_source_dir), QObject(parent)
{
	// ./开头时从workdir下找
	if (_cmd.startsWith("./") || _cmd.startsWith(".\\"))
		cmd = work_dir + "/" + _cmd;
	else
		cmd = _cmd;
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
	process->start(cmd, args,QIODevice::ReadOnly);	
	start_time = QDateTime::currentDateTime();
	return process->waitForStarted();
}

// 只要有一个失败就算失败, 返回错误信息
QString copyDir(const QString& srcPath, const QString& dstPath)
{
	QDir srcDir(srcPath);
	if (!srcDir.exists())
		return "Src not exist.";

	QDir dstDir(dstPath);
	if (!dstDir.exists())
		if (!dstDir.mkpath("."))
			return "Fail to create dest.";

	// 忽略目录
	QStringList ignoreDirs = { ".git", ".idea", ".vs", ".github", "__pycache__"};

	QFileInfoList entries = srcDir.entryInfoList(
		QDir::NoDotAndDotDot | QDir::AllEntries
	);

	for (const QFileInfo& entry : entries)
	{
		QString srcFilePath = entry.absoluteFilePath();
		QString dstFilePath = dstPath + "/" + entry.fileName();

		if (entry.isDir())
		{
			// 忽略指定目录
			if (ignoreDirs.contains(entry.fileName()))
				continue;

			auto ret = copyDir(srcFilePath, dstFilePath);
			if (!ret.isEmpty())
				return ret;
		}
		else
		{
			if (QFile::exists(dstFilePath))
				if (!QFile::remove(dstFilePath))
					return "Fail to remove " + dstFilePath;
;
			if (!QFile::copy(srcFilePath, dstFilePath))
				return "Fail to copy "+srcFilePath+"->"+dstFilePath;
		}
	}
	return "";
}

bool Program::Fetch()
{
	if (enable)
	{
		QMessageBox::information(nullptr, "No", "Stop process before re-fetch");
		return false;
	}
	auto ret = copyDir(source_dir, work_dir);
	if (!ret.isEmpty())
		QMessageBox::warning(nullptr, "Error", ret);
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
	if (channel == QProcess::StandardOutput || ignore_log_error==true)
		tmp = process->readAllStandardOutput();
	else
	{
		tmp = process->readAllStandardError();
		log_error += tmp;
		tmp = "<font color=\"#FF0000\">" + tmp + "</font>";
		has_error = true;
	}
	log_merged += tmp;
	if (log_merged.length() > 10*30000)
		log_merged=log_merged.right(30000);
	if (log_error.length() > 10 * 30000)
		log_error = log_error.right(30000);
	emit signalLogChanged();
}

void Program::OnFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (exitCode != 0 || exitStatus != QProcess::NormalExit)
		LocalLog("Start");
}