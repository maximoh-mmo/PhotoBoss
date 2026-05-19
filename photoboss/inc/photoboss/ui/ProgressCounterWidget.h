#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include "ui/ShaderSpinnerWidget.h"

namespace photoboss {
	
	class ProgressCounterWidget : public QFrame
	{
		enum ProgressState {
			NotStarted,
			InProgress,
			Finished
		};

		Q_OBJECT
	public:
		explicit ProgressCounterWidget(const QString& title, QWidget* parent = nullptr);
		void setProgress(int progress);
		void setTotal(int total);
		void reset();
		void prepareForScan();
		void showSpinner();
		void stop();

	private:
		void buildUi();
		void setColour(QColor color);

		ProgressState m_state_ = NotStarted;
		int m_progress_ = 0;
		int m_total_ = 0;
		QString m_title_;
		ShaderSpinnerWidget* m_spinner_ = nullptr;
		QLabel* m_titleLabel_ = nullptr;
		QLabel* m_progressLabel_ = nullptr;

	};
}