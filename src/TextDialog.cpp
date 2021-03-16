#include "TextDialog.h"
#include <QBoxLayout>
#include <QByteArray>
#include <QScrollBar>
TextDialog::TextDialog(Program* _program, QProcess::ProcessChannel _channel, QWidget *parent):
QDialog(parent),program(_program),channel(_channel)
{
	auto layout = new QHBoxLayout(this);
	editor = new QTextEdit();
	layout->addWidget(editor);
	setAttribute(Qt::WA_DeleteOnClose, true);
	OnLogChanged();
	connect(program, &Program::signalLogChanged, this, &TextDialog::OnLogChanged);
	resize(600, 900);
	editor->verticalScrollBar()->triggerAction(QAbstractSlider::SliderAction::SliderToMaximum);
	editor->setAcceptRichText(true);
}

void TextDialog::OnLogChanged()
{
	int tmp=editor->verticalScrollBar()->sliderPosition();
	QString text = QString::fromLocal8Bit(channel == QProcess::StandardOutput ? program->log_merged : program->log_error);
	text.replace("\n", "<br>");
	editor->setHtml(text);
	editor->verticalScrollBar()->setSliderPosition(tmp);
}
