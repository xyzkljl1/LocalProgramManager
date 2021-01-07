#include "TextDialog.h"
#include <QBoxLayout>
#include <QByteArray>
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
	editor->setText(program->log);
}
