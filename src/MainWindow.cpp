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
	resize(600, 800);
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
	programs.push_back(new Program("Video Downloader","E:/MyWebsiteHelper/Bin/YoutubeDLServer","python", { "-u","__main__.py"},this));
	programs.push_back(new Program("DLSite Downloader", "E:/MyWebsiteHelper/Bin/DLSiteHelperServer", "E:/MyWebsiteHelper/Bin/DLSiteHelperServer/DLSiteHelperServer.exe", {"-u"},this));
	programs.push_back(new Program("PixivAss", "E:/MyWebsiteHelper/Bin/PixivAss", "E:/MyWebsiteHelper/Bin/PixivAss/PixivAss.exe", {}, this));
	programs.push_back(new Program("SSR", "E:/MyWebsiteHelper/Bin/ShadowSocksR", "E:/MyWebsiteHelper/Bin/ShadowSocksR/ShadowsocksR-dotnet4.0.exe", {}, this));
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
		layout->addWidget(btn_0);
		layout->addWidget(btn_1);
		layout->addWidget(btn_2);
		table->setCellWidget(row, Col_Button, widget);
		connect(btn_0, &QCheckBox::stateChanged, this, std::bind(&MainWindow::onSwitch, this, row));
		connect(btn_1, &QPushButton::clicked, this, std::bind(&MainWindow::onRestart, this, row));
		connect(btn_2, &QPushButton::clicked, this, std::bind(&MainWindow::onShowLog, this, row));
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
	auto dialog = new TextDialog(programs[row], this);
	dialog->show();
}

void MainWindow::onRestart(int row)
{
	programs[row]->Restart();
}

void MainWindow::onSwitch(int row)
{
	programs[row]->enable = !programs[row]->enable;
}
