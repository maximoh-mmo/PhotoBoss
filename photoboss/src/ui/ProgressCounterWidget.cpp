#include "ui/ProgressCounterWidget.h"
#include <QLabel.h>
#include <qstyle.h>

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
		m_progressLabel_->setFixedHeight(16);
		m_progressLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_progressLabel_->setScaledContents(false);

		m_spinner_ = new ShaderSpinnerWidget(this);
		// Make spinner square matching twice the label height
		int labelHeight = m_titleLabel_->sizeHint().height();
		int spinnerSize = labelHeight * 2;
		m_spinner_->setFixedSize(spinnerSize, spinnerSize);
		QColor parentBg = this->palette().color(QPalette::Window);
        m_spinner_->setStyleSheet(QString("background-color: %1;").arg(parentBg.name()));
		QSizePolicy sp_retain = m_spinner_->sizePolicy();
		sp_retain.setRetainSizeWhenHidden(true);
		m_spinner_->setSizePolicy(sp_retain);

		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->setAlignment(Qt::AlignLeft);
		layout->setContentsMargins(2,2,2,2);
		layout->setSpacing(0);
		layout->addWidget(m_spinner_);
		QWidget* textContainer = new QWidget(this);
		QVBoxLayout* textLayout = new QVBoxLayout(textContainer);
		textLayout->setContentsMargins(2, 2, 2, 2);
		textLayout->setSpacing(0);
		textLayout->addWidget(m_titleLabel_);
		textLayout->addWidget(m_progressLabel_);
		textContainer->setLayout(textLayout);

		layout->addWidget(textContainer);
		setColour(Qt::gray);
		m_spinner_->hide();

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
		if (m_total_ > 0 && m_progress_ > 0) {
			m_progressLabel_->setText(QString::number(m_progress_) + " / " + QString::number(m_total_));
			if (m_progress_ == m_total_ && m_state_ != Finished) {
				m_state_ = Finished;
				m_spinner_->stop();
				setColour(Qt::green);
			}
		}
	}

	void ProgressCounterWidget::setColour(QColor colour) {
		m_spinner_->setColor(colour);
		this->setStyleSheet("color : " + colour.name() + ";");
	}

	void ProgressCounterWidget::reset() 
	{
		m_spinner_->stop();
		m_spinner_->hide();
		setColour(Qt::gray);
		m_progress_ = 0;
		m_total_ = 0;
		m_progressLabel_->setText("0");
		m_state_ = NotStarted;
	}

	void ProgressCounterWidget::prepareForScan()
	{
		m_spinner_->show();
		setColour(Qt::gray);
		m_progress_ = 0;
		m_total_ = 0;
		m_progressLabel_->setText("0");
		m_state_ = NotStarted;
	}

	void ProgressCounterWidget::showSpinner()
	{
		m_spinner_->show();
	}

	void ProgressCounterWidget::stop()
	{
		m_spinner_->stop();
		setColour(Qt::gray);
	}
}