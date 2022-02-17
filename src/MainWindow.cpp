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
#include "TextDialog.h"
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
	QAction *exit_action = new QAction(QString::fromLocal8Bit("退出"), menu);
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
	//init
	//python需要使用-u关闭stdout缓冲，否则不能及时接收到输出
	//C++内部使用setvbuf关闭缓冲，YoutubeDLServer通过-u参数设置
	//exe必须用完整路径(Why?)
#ifndef _DEBUG
	programs.push_back(new Program("MyDownloader","E:/MyWebsiteHelper/Bin/MyWebDownloadServer","python", { "-u","__main__.py"},this));
	programs.push_back(new Program("DLSite Downloader", "E:/MyWebsiteHelper/Bin/DLSiteHelperServer", "E:/MyWebsiteHelper/Bin/DLSiteHelperServer/DLSiteHelperServer.exe", {"-u"},this));
	programs.push_back(new Program("PixivAss", "E:/MyWebsiteHelper/Bin/PixivAss", "E:/MyWebsiteHelper/Bin/PixivAss/PixivAss.exe", {}, this));
	programs.push_back(new Program("JASMR Downloader", "E:/MyWebsiteHelper/Bin/MySpider/japaneseasmr.com", "E:/MyWebsiteHelper/Bin/MySpider/japaneseasmr.com/japaneseasmr.com.exe", { "-u" }, this));
	programs.push_back(new Program("ASMRONE Downloader", "E:/MyWebsiteHelper/Bin/MySpider/asmr.one", "E:/MyWebsiteHelper/Bin/MySpider/asmr.one/asmr.one.exe", { "-u" }, this));
	//IDM不能用此程序管理,启动的进程会变为not running从而导致一直重启
//	programs.push_back(new Program("IDM", "C:/Program Files (x86)/Internet Download Manager" , "C:/Program Files (x86)/Internet Download Manager/IDMan.exe", {}, this));
#else
	programs.push_back(new Program("Test", "E:/MyWebsiteHelper/TmpProject/TmpProject/Debug", "E:/MyWebsiteHelper/TmpProject/TmpProject/Debug/Project1.exe", {"-u"}, this));
	//programs.push_back(new Program("IDM", "C:/Program Files (x86)/Internet Download Manager", "C:/Program Files (x86)/Internet Download Manager/IDMan.exe", {"-Embedding"}, this));
	//programs.push_back(new Program("Test", "E:/MyWebsiteHelper/QtConsoleApplication1/x64/Release", "E:/MyWebsiteHelper/QtConsoleApplication1/x64/Release/QtConsoleApplication1.exe", { }, this));
#endif
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
		layout->addWidget(btn_0);
		layout->addWidget(btn_1);
		layout->addWidget(btn_2);
		layout->addWidget(btn_3);
		table->setCellWidget(row, Col_Button, widget);
		connect(btn_0, &QCheckBox::stateChanged, this, std::bind(&MainWindow::onSwitch, this, row));
		connect(btn_1, &QPushButton::clicked, this, std::bind(&MainWindow::onRestart, this, row));
		connect(btn_2, &QPushButton::clicked, this, std::bind(&MainWindow::onShowLog, this, row));
		connect(btn_3, &QPushButton::clicked, this, std::bind(&MainWindow::onShowError, this, row));
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
			setVisible(false);
		else
		{
			setVisible(true);
			SetWindowPos((HWND)winId(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		}
	}
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	e->ignore();
	hide();
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

void MainWindow::onSwitch(int row)
{
	programs[row]->enable = !programs[row]->enable;
}
