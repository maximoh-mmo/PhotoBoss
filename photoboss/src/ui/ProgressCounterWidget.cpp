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

		m_spinner_ = new WaitingSpinnerWidget(this,true,false);
		m_spinner_->hide();

		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->setContentsMargins(2, 2, 2, 2);
		layout->setSpacing(6);

		layout->addWidget(m_spinner_);

		QWidget* textContainer = new QWidget(this);
		QVBoxLayout* textLayout = new QVBoxLayout(textContainer);
		textLayout->setContentsMargins(2, 2, 2, 2);
		textLayout->setSpacing(6);
		
		textLayout->addWidget(m_titleLabel_);
		textLayout->addWidget(m_progressLabel_);
		textContainer->setLayout(textLayout);

		layout->addWidget(textContainer);
		setColour(Qt::gray);
	}

	void ProgressCounterWidget::setProgress(int progress)
	{
		m_progress_ = progress;
		if (!m_active_) {
			if (m_progress_ > 0) {
				setActive(true);
			}
		}
		if (m_total_ == 0) {
			m_progressLabel_->setText(QString::number(m_progress_));
			return;
		}
		else {
			m_progressLabel_->setText(QString::number(m_progress_) + " / " + QString::number(m_total_));
			if (m_progress_ >= m_total_) {
				m_spinner_->stop();
				setColour(Qt::green);
			}
		}
	}
	void ProgressCounterWidget::setTotal(int total)
	{
		m_total_ = total;
		if (m_total_ == 0) {
			m_progressLabel_->setText(QString::number(m_progress_));
			return;
		}
		else {
			m_progressLabel_->setText(QString::number(m_progress_) + " / " + QString::number(m_total_));
			if (m_progress_ >= m_total_) {
				m_spinner_->stop();
				setColour(Qt::green);
			}
		}
	}

	void ProgressCounterWidget::setActive(bool active)
	{
		m_active_ = active;
		if (m_active_) {
			m_spinner_->start();
			m_spinner_->show();
			setColour(Qt::yellow);
		}
		else {
			m_spinner_->stop();
			m_spinner_->hide();
		}
	}
	void ProgressCounterWidget::setColour(QColor colour) {
		m_spinner_->setColor(colour);
		this->setStyleSheet("color : " + colour.name() + ";");
	}

	void ProgressCounterWidget::finish()
	{
		m_spinner_->stop();
		setColour(Qt::green);
	}
	void ProgressCounterWidget::reset(int total) 
	{
		m_spinner_->hide();
		setColour(Qt::gray);
		m_progress_ = 0;
		m_total_ = total;
		m_progressLabel_->setText("0");
	}
}