#include "MainWindow.h"
#include <windows.h>
#include <Winuser.h>
#include <QMenu>
#include <QTimer>
#include <QCheckBox>
#include <QBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSet>
#include "Control.h"
#include "TextDialog.h"

namespace {

QString resolveDirectory(const QDir& configDirectory, const QString& path)
{
	return QDir::cleanPath(QDir::isRelativePath(path)
		? configDirectory.absoluteFilePath(path) : path);
}

QString loadPrograms(QObject* parent, std::vector<Program*>& programs)
{
	const QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.json");
	QFile file(configPath);
	if (!file.open(QIODevice::ReadOnly))
		return QStringLiteral("无法读取配置文件：%1\n%2").arg(configPath, file.errorString());

	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
	if (parseError.error != QJsonParseError::NoError)
		return QStringLiteral("配置文件不是有效的 JSON：%1").arg(parseError.errorString());
	if (!document.isObject())
		return QStringLiteral("配置文件根节点必须是对象。");

	const QJsonObject root = document.object();
	const QString workRootValue = root.value("workRoot").toString();
	const QString logRootValue = root.value("logRoot").toString();
	if (workRootValue.isEmpty() || logRootValue.isEmpty() || !root.value("programs").isArray())
		return QStringLiteral("配置文件必须包含 workRoot、logRoot 和 programs 数组。");

	const QDir configDirectory = QFileInfo(configPath).absoluteDir();
	const QString workRoot = resolveDirectory(configDirectory, workRootValue);
	const QString logRoot = resolveDirectory(configDirectory, logRootValue);
	QSet<QString> names;
	const QJsonArray entries = root.value("programs").toArray();
	for (int index = 0; index < entries.size(); ++index)
	{
		if (!entries[index].isObject())
			return QStringLiteral("programs[%1] 必须是对象。").arg(index);
		const QJsonObject entry = entries[index].toObject();
		const QString name = entry.value("name").toString();
		const QString sourceDir = entry.value("sourceDir").toString();
		const QString command = entry.value("command").toString();
		if (name.isEmpty() || sourceDir.isEmpty() || command.isEmpty())
			return QStringLiteral("programs[%1] 必须包含 name、sourceDir 和 command。").arg(index);

		const QString nameKey = name.toCaseFolded();
		if (names.contains(nameKey))
			return QStringLiteral("程序名称不能重复：%1").arg(name);
		names.insert(nameKey);

		const QJsonValue argumentsValue = entry.value("arguments");
		if (!argumentsValue.isUndefined() && !argumentsValue.isArray())
			return QStringLiteral("程序 %1 的 arguments 必须是数组。").arg(name);
		QStringList arguments;
		for (const QJsonValue argument : argumentsValue.toArray())
		{
			if (!argument.isString())
				return QStringLiteral("程序 %1 的 arguments 只能包含字符串。").arg(name);
			arguments.push_back(argument.toString());
		}

		programs.push_back(new Program(name,
			resolveDirectory(configDirectory, sourceDir),
			QDir(workRoot).filePath(name), command, arguments, parent,
			entry.value("ignoreLogError").toBool(false)));
	}
	Program::InitializeLogs(logRoot);
	return {};
}

}

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
	setWindowIcon(QIcon(":/asset/logo.png"));
	resize(1200, 800);
	//托盘
	QSystemTrayIcon* icon = new QSystemTrayIcon(this);
	icon->setIcon(QIcon(":/asset/logo.png"));
	icon->setToolTip("LocalProgramManager");
	icon->show();
	connect(icon, &QSystemTrayIcon::activated, this, &MainWindow::onIconClicked);
	QMenu *menu = new QMenu();
	QAction *exit_action = new QAction(QStringLiteral("退出"), menu);
	menu->addAction(exit_action);
	icon->setContextMenu(menu);
	connect(exit_action, &QAction::triggered, this, &MainWindow::signalClose);
	//列表
	table = new QTableWidget();
	setCentralWidget(table);
	table->setColumnCount(Col_Count);
	table->setHorizontalHeaderLabels(QStringList{"Name","Path","PID","Living","Status","Op"});
	table->horizontalHeader()->setSectionsClickable(false);
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->setColumnWidth(Col_Pid, 20);
	table->setColumnWidth(Col_Status, 40);
	const QString configError = loadPrograms(this, programs);
	if (!configError.isEmpty())
	{
		for (auto* program : programs)
			delete program;
		programs.clear();
		QMessageBox::critical(this, QStringLiteral("配置错误"), configError);
	}
	for (auto& program : programs)
		connect(program, &Program::signalErrorChanged, this, &MainWindow::updateTable);
	QTimer* timer = new QTimer(this);
	timer->setInterval(3000);
	timer->setSingleShot(false);
	connect(timer, &QTimer::timeout, this, &MainWindow::RegularMaintain);
	timer->start();
}

void MainWindow::initTable()
{
	table->setRowCount(0);
	table->setRowCount((int)programs.size());
	for (int row = 0; row < (int)programs.size(); ++row)
	{
		table->setItem(row, Col_Name, new QTableWidgetItem(programs[row]->name));
		table->setItem(row, Col_Path, new QTableWidgetItem(programs[row]->work_dir));
		table->setItem(row, Col_Pid, new QTableWidgetItem("-1"));
		table->setItem(row, Col_Living, new QTableWidgetItem("-1"));
		table->setItem(row, Col_Status, new QTableWidgetItem("none"));
		auto widget = new QWidget(table);
		widget->setContentsMargins(0,0,0,0);
		auto layout = new QHBoxLayout(widget);
		layout->setContentsMargins(0, 0, 0, 0);
		auto btn_0 = new QCheckBox("Valid");
		btn_0->setChecked(true);
		auto btn_1 = new QPushButton("R");
		auto btn_2 = new QPushButton("L");
		auto btn_3 = new QPushButton("E");
		auto btn_4 = new QPushButton("F");
		layout->addWidget(btn_0);
		layout->addWidget(btn_1);
		layout->addWidget(btn_2);
		layout->addWidget(btn_3);
		layout->addWidget(btn_4);
		table->setCellWidget(row, Col_Button, widget);
		connect(btn_0, &QCheckBox::stateChanged, this, std::bind(&MainWindow::onSwitch, this, row));
		connect(btn_1, &QPushButton::clicked, this, std::bind(&MainWindow::onRestart, this, row));
		connect(btn_2, &QPushButton::clicked, this, std::bind(&MainWindow::onShowLog, this, row));
		connect(btn_3, &QPushButton::clicked, this, std::bind(&MainWindow::onShowError, this, row));
		connect(btn_4, &QPushButton::clicked, this, std::bind(&MainWindow::onFetch, this, row));
	}
}

void MainWindow::updateTable() {
	//programs是有序的
	if (table->rowCount() != (int)programs.size())
		initTable();
	for (int row = 0; row < (int)programs.size(); ++row)
	{
		//名字和路径不会改变
		table->item(row, Col_Pid)->setText(QString("%1").arg(programs[row]->PID()));
		auto t = programs[row]->enable?programs[row]->start_time.secsTo(QDateTime::currentDateTime()):0;
		table->item(row, Col_Living)->setText(QString("%1:%2:%3").arg(t/3600).arg((t/60)%60).arg(t%60));
		table->item(row, Col_Status)->setText(programs[row]->StatusText());
		QColor color = programs[row]->has_error ? QColor(222, 22, 22) : QColor(255, 255, 255,0);
		for (int col = 0; col < (int)Col_Count; ++col)
			if(table->item(row,col))
				table->item(row, col)->setBackgroundColor(color);
	}
}

void MainWindow::onIconClicked(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::ActivationReason::DoubleClick)
	{
		if (isVisible())
			hideAndCloseChildDialog();
		else
		{
			setVisible(true);
			SetWindowPos((HWND)winId(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		}
	}
}

void MainWindow::hideAndCloseChildDialog()
{
	for (auto dialog : this->findChildren<TextDialog*>())
		dialog->close();
	hide();
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	e->ignore();
	hideAndCloseChildDialog();
}

QByteArray MainWindow::HandleControlMessage(const Control::Request& request)
{
	QJsonObject response;
	response["success"] = false;
	response["command"] = request.command;
	if (!request.program.isEmpty())
		response["program"] = request.program;

	auto finish = [&](int exitCode, const QString& responseMessage) {
		this->updateTable();
		response["exitCode"] = exitCode;
		response["message"] = responseMessage;
		return QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n";
	};

	if (request.command != "deploy" && request.command != "restart")
		return finish(Control::InvalidArguments, "Unsupported control command.");

	Program* selected = nullptr;
	for (auto program : programs)
	{
		if (program->name.compare(request.program, Qt::CaseInsensitive) == 0)
		{
			selected = program;
			break;
		}
	}
	if (!selected)
		return finish(Control::ProgramNotFound, "Unknown program name.");

	response["program"] = selected->name;
	if (selected->deploying)
		return finish(Control::AlreadyDeploying, "A deployment is already running for this program.");

	if (request.command == "restart")
	{
		const bool restarted = selected->Restart();
		response["success"] = restarted;
		response["pid"] = selected->PID();
		return finish(restarted ? Control::Success : Control::OperationFailed,
			restarted ? "Program restarted." : "Program failed to restart.");
	}
	QString deployMessage;
	const bool deployed = selected->Deploy(deployMessage);
	response["success"] = deployed;
	response["pid"] = selected->PID();
	return finish(deployed ? Control::Success : Control::OperationFailed, deployMessage);
}

void MainWindow::RegularMaintain()
{
	for (unsigned int i = 0; i < programs.size(); ++i)
		programs[i]->Check();
	updateTable();
}

void MainWindow::onShowLog(int row)
{
	auto dialog = new TextDialog(programs[row],QProcess::StandardOutput, this);
	dialog->show();
}

void MainWindow::onShowError(int row)
{
	auto dialog = new TextDialog(programs[row], QProcess::StandardError, this);
	dialog->show();
	programs[row]->ClearError();
}

void MainWindow::onRestart(int row)
{
	programs[row]->Restart();
}

void MainWindow::onFetch(int row)
{
	programs[row]->Fetch();
}

void MainWindow::onSwitch(int row)
{
	programs[row]->enable = !programs[row]->enable;
}

