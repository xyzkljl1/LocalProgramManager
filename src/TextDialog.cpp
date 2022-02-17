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
	//��Ȼ�����������ڵ�closeEventʹ֮��hide�����Ǵ���־����->�ر�������->�ر���־���ڻᵼ�³����˳���ͬʱ�ֲ�ϣ����־���ڹر�ʱ��hide(ϣ����ر�)����˽���־���ڸ�Ϊģ̬����ֹ�������ڹر�ǰ�ر�
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
