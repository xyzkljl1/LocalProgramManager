#include "Program.h"
#include <Windows.h>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace {
QString programLogDirectory;
}

void Program::InitializeLogs(const QString& directoryPath)
{
    programLogDirectory = QDir::cleanPath(directoryPath);
    QDir directory(programLogDirectory);
    if (!directory.mkpath("."))
    {
        qWarning() << "Cannot create log directory:" << directory.absolutePath();
        return;
    }

    const QDate cutoff = QDate::currentDate().addDays(-30);
    const QRegularExpression pattern(R"(\A(\d{4}-\d{2}-\d{2})_.+\.log\z)");
    const auto entries = directory.entryInfoList(QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo& entry : entries)
    {
        const auto match = pattern.match(entry.fileName());
        if (!match.hasMatch())
            continue;
        const QDate date = QDate::fromString(match.captured(1), "yyyy-MM-dd");
        if (date.isValid() && date < cutoff && !QFile::remove(entry.absoluteFilePath()))
            qWarning() << "Cannot remove expired log:" << entry.absoluteFilePath();
    }
}

void Program::AppendLog(const QByteArray& data)
{
    if (data.isEmpty())
        return;

    QDir directory(programLogDirectory);
    if (!directory.mkpath("."))
    {
        qWarning() << "Cannot create log directory:" << directory.absolutePath();
        return;
    }

    QString fileName = name;
    fileName.replace(QRegularExpression(R"([<>:"/\\|?*\x00-\x1F])"), "_");
    if (fileName.isEmpty())
        fileName = "program";
    fileName = QDate::currentDate().toString("yyyy-MM-dd") + "_" + fileName + ".log";
    QFile file(directory.filePath(fileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qWarning() << "Cannot open log file:" << file.fileName() << file.errorString();
        return;
    }
    if (file.write(data) != data.size() || !file.flush())
        qWarning() << "Cannot write log file:" << file.fileName() << file.errorString();
}

Program::Program(const QString& _name, const QString& _source_dir, const QString& _work_dir, const QString& _cmd, const QStringList& _args, QObject* parent,bool ignoreLogError) :
	name(_name),work_dir(_work_dir),args(_args),ignore_log_error(ignoreLogError), source_dir(_source_dir), QObject(parent)
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
	const auto entry = name + ":" + message + " on " + QDateTime::currentDateTime().toString();
	AppendLog((entry + "\r\n").toLocal8Bit());
	auto text = ("<font color=\"#0000FF\">" + entry + "</font>\r\n").toLocal8Bit();
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
	return ret.isEmpty();
}

bool Program::Deploy(QString& message)
{
	if (deploying)
	{
		message = "A deployment is already running for this program.";
		return false;
	}

	const QString sourcePath = QDir::cleanPath(QDir(source_dir).absolutePath()).replace('\\', '/');
	const QString workPath = QDir::cleanPath(QDir(work_dir).absolutePath()).replace('\\', '/');
	if (!QDir(sourcePath).exists())
	{
		message = "Source directory does not exist: " + source_dir;
		return false;
	}
	if (sourcePath.compare(workPath, Qt::CaseInsensitive) == 0
		|| sourcePath.startsWith(workPath + "/", Qt::CaseInsensitive)
		|| workPath.startsWith(sourcePath + "/", Qt::CaseInsensitive))
	{
		message = "Source and working directories must be separate and cannot contain one another.";
		return false;
	}

	const QString commandPath = QDir::cleanPath(cmd).replace('\\', '/');
	if (commandPath.startsWith(workPath + "/", Qt::CaseInsensitive))
	{
		const QString relativeCommand = commandPath.mid(workPath.length() + 1);
		if (!QFileInfo(QDir(sourcePath).filePath(relativeCommand)).isFile())
		{
			message = "The source directory does not contain the configured executable: " + relativeCommand;
			return false;
		}
	}

	deploying = true;
	const bool wasEnabled = enable;
	LocalLog("Deploy stopping");
	Stop();
	LocalLog("Deploy copying");
	const QString copyError = copyDir(source_dir, work_dir);
	if (!copyError.isEmpty())
	{
		message = "Deployment copy failed: " + copyError;
		if (wasEnabled)
			message += Start() ? " The program was restarted from the current working directory."
				: " The program could not be restarted.";
		deploying = false;
		return false;
	}

	if (wasEnabled)
	{
		LocalLog("Deploy starting");
		const bool started = Start();
		const bool exitedDuringCheck = started && process && process->waitForFinished(3000);
		const bool runningAfterCheck = started && process
			&& !exitedDuringCheck && process->state() == QProcess::Running;
		if (!runningAfterCheck)
		{
			message = "The deployed process did not remain running during the three-second startup check.";
			deploying = false;
			return false;
		}
	}

	LocalLog("Deploy succeeded");
	message = wasEnabled ? "Deployment succeeded and the program is running."
		: "Deployment succeeded; the program remains disabled.";
	deploying = false;
	return true;
}

void Program::Check()
{
	if (deploying)
		return;
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
	{
		// Persist ignored stderr without changing the existing UI/error behavior.
		if (channel == QProcess::StandardError)
			AppendLog(process->readAllStandardError());
		tmp = process->readAllStandardOutput();
		AppendLog(tmp);
	}
	else
	{
		tmp = process->readAllStandardError();
		AppendLog(tmp);
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
