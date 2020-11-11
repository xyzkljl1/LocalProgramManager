#pragma once
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSystemTrayIcon>
//#include <QtWidgets/qtablewidget.h>
#include <QDateTime>
#include <QTableWidget>
#include "Program.h"
class MainWindow :public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
signals:
	void signalClose();
protected:
	void RegularMaintain();
	enum Col {
		Col_Name=0,
		Col_Path,
		Col_Pid,
		Col_Living,
		Col_Status,
		Col_Button,
		Col_Count
	};
	void initTable();
	void updateTable();
	void onIconClicked(QSystemTrayIcon::ActivationReason);
	void onShowLog(int row);
	void onRestart(int row);
	void onShowWindow(int row);
	void closeEvent(QCloseEvent * event)override;
	QTableWidget* table=nullptr;
	std::vector<Program*> programs;
};