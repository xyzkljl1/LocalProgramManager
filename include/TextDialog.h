#pragma once
#include <QDialog>
#include <QTextEdit>
#include "Program.h"
class TextDialog :public QDialog {
	Q_OBJECT
public:
	TextDialog(Program* program,QWidget *parent = nullptr);
protected:
	void OnLogChanged();
protected:
	QTextEdit* editor;
	Program* program=nullptr;
};