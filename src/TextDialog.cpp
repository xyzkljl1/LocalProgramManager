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
	editor->setAcceptRichText(true);
	//虽然重载了主窗口的closeEvent使之仅hide，但是打开日志窗口->关闭主窗口->关闭日志窗口会导致程序退出，同时又不希望日志窗口关闭时仅hide(希望其关闭)，因此将日志窗口改为模态，防止在主窗口关闭前关闭
	setModal(true);
}

void TextDialog::OnLogChanged()
{
	int tmp=editor->verticalScrollBar()->sliderPosition();
	QString text = QString::fromLocal8Bit(channel == QProcess::StandardOutput ? program->log_merged : program->log_error);
	text.replace("\n", "<br>");
	editor->setHtml(text);
	editor->verticalScrollBar()->setSliderPosition(tmp);
}

void TextDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	editor->verticalScrollBar()->triggerAction(QAbstractSlider::SliderAction::SliderToMaximum);
}
