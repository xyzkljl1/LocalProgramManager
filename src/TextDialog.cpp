#include "TextDialog.h"
#include <QBoxLayout>
#include <QByteArray>
#include <QScrollBar>
TextDialog::TextDialog(Program* _program, QWidget *parent):
QDialog(parent),program(_program)
{
	auto layout = new QHBoxLayout(this);
	editor = new QTextEdit();
	layout->addWidget(editor);
	setAttribute(Qt::WA_DeleteOnClose, true);
	OnLogChanged();
	connect(program, &Program::LogChanged, this, &TextDialog::OnLogChanged);
}

void TextDialog::OnLogChanged()
{
	int tmp=editor->verticalScrollBar()->sliderPosition();
	editor->setText(QString::fromLocal8Bit(program->log));
	editor->verticalScrollBar()->setSliderPosition(tmp);
}
