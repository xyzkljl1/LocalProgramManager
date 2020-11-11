#include "TextDialog.h"
#include <QBoxLayout>
#include <QTextEdit>
TextDialog::TextDialog(const QByteArray&_data, QWidget *parent):
QDialog(parent),data(_data)
{
	auto layout = new QHBoxLayout(this);
	auto editor = new QTextEdit();
	layout->addWidget(editor);
	editor->setText(_data);
	setAttribute(Qt::WA_DeleteOnClose, true);
}