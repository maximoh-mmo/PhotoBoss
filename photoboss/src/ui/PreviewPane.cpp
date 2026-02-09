#include "ui/PreviewPane.h"
#include "util/humanSize.h"

namespace photoboss
{
	PreviewPane::PreviewPane(QWidget* parent) : QWidget(parent)
	{
		m_scrollArea = new QScrollArea;
		m_meta_label_ = new QLabel;
		m_image_label_ = new QLabel;
	}

	void PreviewPane::showImage(const ImageEntry& entry) {
		QPixmap pix(entry.path);
		if (pix.isNull())
			return;
		m_image_label_->setPixmap(
			pix.scaled(
				m_scrollArea->viewport()->size(),
				Qt::KeepAspectRatio,
				Qt::SmoothTransformation
			)
		);

		m_meta_label_->setText(
			QString("%1 x %2 | %3")
			.arg(entry.resolution.width())
			.arg(entry.resolution.height())
			.arg(humanSize::size(entry.fileSize))
		);
	}
}