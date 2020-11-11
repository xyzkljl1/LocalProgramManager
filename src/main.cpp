#include <QApplication>
#include "MainWindow.h"
int main(int argc,char**argv) {
	QApplication app(argc,argv);
	MainWindow window;
	window.show();
	QObject::connect(&window,&MainWindow::signalClose,&app,&QApplication::quit);
	return app.exec();
}