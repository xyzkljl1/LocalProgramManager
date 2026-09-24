#include <QApplication>
#include <QDebug>
#include <Windows.h>
#include "Control.h"
#include "MainWindow.h"
int main(int argc,char**argv) {
	if (Control::IsControlMode(argc, argv))
		return Control::RunClient(argc, argv);

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
	Control::Server controlServer;
	if (!controlServer.Start([&window](const Control::Request& request) {
		return window.HandleControlMessage(request);
	}))
		qWarning() << "Cannot start the local control endpoint:" << controlServer.ErrorString();
	window.show();
	QObject::connect(&window, &MainWindow::signalClose, &app, &QApplication::quit);
	const int result = app.exec();
	CloseHandle(mutex);
	return result;
}
