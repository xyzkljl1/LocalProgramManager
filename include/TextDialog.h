#pragma once
#include <QDialog>
#include <QTextEdit>
#include "Program.h"
class TextDialog :public QDialog {
	Q_OBJECT
public:
	TextDialog(Program* program, QProcess::ProcessChannel channel,QWidget *parent = nullptr);
protected:
	void OnLogChanged();
	void showEvent(QShowEvent *) override;
protected:
	QTextEdit* editor;
	QProcess::ProcessChannel channel;
	Program* program=nullptr;
};