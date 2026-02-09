#include "ui/ImageThumbWidget.h"
#include "util/humanSize.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <qstyle.h>

namespace photoboss {
	ImageThumbWidget::ImageThumbWidget(QWidget* parent) : QFrame(parent)
	{
		setObjectName("ImageThumbWidget");
		setFrameShape(QFrame::NoFrame);
		setCursor(Qt::PointingHandCursor);

		buildUi();
		updateVisualState();
	}

    void ImageThumbWidget::buildUi()
    {
        m_checkBox_ = new QCheckBox(this);

        m_thumbnailLabel_ = new QLabel(this);
        m_thumbnailLabel_->setAlignment(Qt::AlignCenter);
        m_thumbnailLabel_->setFixedSize(140, 140);
        m_thumbnailLabel_->setScaledContents(true);

        m_sizeLabel_ = new QLabel(this);
        m_resolutionLabel_ = new QLabel(this);

        m_keepBadge_ = new QLabel("? KEEP", this);
        m_keepBadge_->setObjectName("keepBadge");

        m_deleteBadge_ = new QLabel("?? DELETE", this);
        m_deleteBadge_->setObjectName("deleteBadge");

        m_keepBadge_->hide();
        m_deleteBadge_->hide();

        auto* topRow = new QHBoxLayout();
        topRow->addWidget(m_checkBox_);
        topRow->addStretch();

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(4);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->addLayout(topRow);
        layout->addWidget(m_thumbnailLabel_);
        layout->addWidget(m_keepBadge_);
        layout->addWidget(m_deleteBadge_);
        layout->addWidget(m_sizeLabel_);
        layout->addWidget(m_resolutionLabel_);

        setLayout(layout);

        connect(m_checkBox_, &QCheckBox::toggled, this, [this](bool checked) {
            setState(checked ? State::Selected : State::Normal);
            emit selectionChanged(checked);
            });
    }
    void ImageThumbWidget::setImage(const ImageEntry* entry)
    {
        m_entry_ = entry;
        if (!m_entry_)
            return;

        // Thumbnail
        QPixmap pix(m_entry_->path);
        if (!pix.isNull()) {
            m_thumbnailLabel_->setPixmap(
                pix.scaled(120, 120,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation)
            );
        }
        QString resolution =
            QString("%1 x %2")
            .arg(m_entry_->resolution.width())
            .arg(m_entry_->resolution.height());

        setMeta(
            humanSize::size(m_entry_->fileSize),
            resolution
        );
    }
    void ImageThumbWidget::setThumbnail(const QPixmap& pixmap)
	{
        m_thumbnailLabel_->setPixmap(pixmap);
	}

    void ImageThumbWidget::setState(State state)
    {
        if (m_state_ == state)
            return;

        m_state_ = state;
        updateVisualState();
    }

	void ImageThumbWidget::setMeta(QString size, QString resolution)
	{
		m_sizeLabel_->setText(size);
		m_resolutionLabel_->setText(resolution);
	}

    void ImageThumbWidget::updateVisualState()
    {
        m_keepBadge_->hide();
        m_deleteBadge_->hide();

        switch (m_state_) {
        case State::Normal:
            m_checkBox_->setChecked(false);
            setProperty("state", "normal");
            break;

        case State::Selected:
            m_checkBox_->setChecked(true);
            m_keepBadge_->show();
            setProperty("state", "selected");
            break;

        case State::MarkedForDelete:
            m_checkBox_->setChecked(false);
            m_deleteBadge_->show();
            setProperty("state", "delete");
            break;
        }

        style()->unpolish(this);
        style()->polish(this);
        update();
    }

    void ImageThumbWidget::mousePressEvent(QMouseEvent* event)
    {
        QFrame::mousePressEvent(event);
        emit clicked(this);
    }

    void ImageThumbWidget::enterEvent(QEnterEvent* event)
    {
        Q_UNUSED(event);
        setProperty("hover", true);
        style()->polish(this);
    }

    void ImageThumbWidget::leaveEvent(QEvent* event)
    {
        Q_UNUSED(event);
        setProperty("hover", false);
        style()->polish(this);
    }
}
