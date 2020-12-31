#include <QApplication>
#include <Windows.h>
#include "MainWindow.h"
int main(int argc,char**argv) {
	//确保该程序只会运行一个
	auto mutex=CreateMutexA(NULL,true,"{A16999D2-6524-4C7D-B193-60D62F451FEE}");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(mutex);
		return 0;
	}
	//似乎没有必要lock?
	QApplication app(argc, argv);
	MainWindow window;
	window.show();
	QObject::connect(&window, &MainWindow::signalClose, &app, &QApplication::quit);
	return app.exec();
}