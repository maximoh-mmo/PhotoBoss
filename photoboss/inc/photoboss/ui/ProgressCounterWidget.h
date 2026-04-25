#pragma once

#include <QWidget.h>
#include <QLabel.h>
#include <qlayout.h>
#include "ui/WaitingSpinnerWidget.h"

namespace photoboss {
	
	class ProgressCounterWidget : public QFrame
	{
		Q_OBJECT
	public:
		explicit ProgressCounterWidget(const QString& title, QWidget* parent = nullptr);
		void setProgress(int progress);
		void setTotal(int total);
		void reset(int total = 0);
		void finish();
	
	private:
		void buildUi();
		void setColour(QColor color);
		void setActive(bool active);

		int m_progress_ = 0;
		int m_total_ = 0;
		QString m_title_;
		WaitingSpinnerWidget* m_spinner_ = nullptr;
		QLabel* m_titleLabel_ = nullptr;
		QLabel* m_progressLabel_ = nullptr;
		QLabel* m_totalLabel_ = nullptr;
		bool m_active_ = false;
	};
}