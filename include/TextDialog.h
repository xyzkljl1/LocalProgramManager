#pragma once
#include <QDialog>
#include <QByteArray>
class TextDialog :public QDialog {
	Q_OBJECT
public:
	TextDialog(const QByteArray&_data,QWidget *parent = nullptr);
protected:
	QByteArray data;
};