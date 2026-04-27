#include "ui/ProgressCounterWidget.h"
#include <QLabel.h>

namespace photoboss
{
	ProgressCounterWidget::ProgressCounterWidget(const QString& title, QWidget* parent) : m_title_(title), QFrame(parent)
	{
		setObjectName("ProgressCounterWidget");
		setFrameShape(QFrame::Box);
		buildUi();
	}

	void ProgressCounterWidget::buildUi() 
	{
		m_titleLabel_ = new QLabel(this);
		m_titleLabel_->setObjectName("titleLabel");
		m_titleLabel_->setText(m_title_);
		m_titleLabel_->setAlignment(Qt::AlignCenter);
		m_titleLabel_->setStyleSheet("font-weight: bold;");
		m_titleLabel_->setFixedHeight(16);
		m_titleLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_titleLabel_->setScaledContents(false);

		m_progressLabel_ = new QLabel(this);
		m_progressLabel_->setObjectName("progressLabel");
		m_progressLabel_->setText("0");
		m_progressLabel_->setAlignment(Qt::AlignCenter);
		m_progressLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_progressLabel_->setScaledContents(false);

		m_spinner_ = new ShaderSpinnerWidget(this);
		m_spinner_->setFixedSize(24, 24);
		m_spinner_->show();

		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(6);

		QWidget* textContainer = new QWidget(this);
		QVBoxLayout* textLayout = new QVBoxLayout(textContainer);
		textLayout->setContentsMargins(2, 2, 2, 2);
		textLayout->setSpacing(6);
		
		textLayout->addWidget(m_titleLabel_);
		textLayout->addWidget(m_progressLabel_);
		textContainer->setLayout(textLayout);

		layout->addWidget(textContainer);
		layout->addWidget(m_spinner_);
		setColour(Qt::gray);
	}

	void ProgressCounterWidget::setProgress(int progress)
	{
		if (progress < 0) {
			progress = 0;
			return;
		}
		if (m_state_ == NotStarted) {
			m_state_ = InProgress;
			m_spinner_->start();
			setColour(Qt::yellow);
		}
		m_progress_ = progress;
		if (m_total_ > 0) {
			m_progressLabel_->setText(QString::number(m_progress_) + " / " + QString::number(m_total_));
			if(m_progress_ == m_total_) {
				if (m_state_ != Finished) {
					m_state_ = Finished;
					m_spinner_->stop();
					setColour(Qt::green);
				}
			}
		}
		else {
			m_progressLabel_->setText(QString::number(m_progress_));
		}
	}
	void ProgressCounterWidget::setTotal(int total)
	{
		m_total_ = total;
	}

	void ProgressCounterWidget::setColour(QColor colour) {
		m_spinner_->setColor(colour);
		this->setStyleSheet("color : " + colour.name() + ";");
	}

	void ProgressCounterWidget::reset() 
	{
		setColour(Qt::gray);
		m_progress_ = 0;
		m_total_ = 0;
		m_progressLabel_->setText("0");
		m_state_ = NotStarted;
	}
	void ProgressCounterWidget::switchState()
	{
		if (m_state_ == NotStarted) {
			setTotal(10);
			setProgress(1);
		}
		else if (m_state_ == InProgress) {
			setProgress(10);
		}
		else {
			reset();
		}
	}
}