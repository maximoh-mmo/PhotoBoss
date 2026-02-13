#include "ui/ImageThumbWidget.h"
#include "util/OrientImage.h"
#include "util/humanSize.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <qstyle.h>
#include "util/AppSettings.h"

namespace photoboss {

	ImageThumbWidget::ImageThumbWidget(const ImageEntry entry, QWidget* parent) : m_entry_(entry), QFrame(parent)
	{
        setObjectName("ImageThumbWidget");
		setFrameShape(QFrame::NoFrame);
		setCursor(Qt::PointingHandCursor);

		buildUi();
		update();
	}

    void ImageThumbWidget::buildUi()
    {
        m_thumbnailLabel_ = new QLabel(this);
        m_thumbnailLabel_->setAlignment(Qt::AlignCenter);
        m_thumbnailLabel_->setFixedSize(140, 140);
        m_thumbnailLabel_->setScaledContents(false);

        m_checkBox_ = new QCheckBox(this);
        m_keepBadge_ = new QLabel("✓ KEEP", this);
        m_keepBadge_->setObjectName("keepBadge");
        m_deleteBadge_ = new QLabel("🗑️ DELETE", this);
        m_deleteBadge_->setObjectName("deleteBadge");
        m_keepBadge_->hide();
        m_deleteBadge_->hide();

        m_sizeLabel_ = new QLabel(humanSize(m_entry_.fileSize), this);

        m_resolutionLabel_ = new QLabel(
            QString("%1 x %2")
            .arg(m_entry_.resolution.width())
            .arg(m_entry_.resolution.height())
            , this
        );

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(6);
        layout->addWidget(m_checkBox_, 0, Qt::AlignHCenter);
        layout->addWidget(m_thumbnailLabel_, 0, Qt::AlignHCenter);
        layout->addWidget(m_keepBadge_, 0, Qt::AlignHCenter);
        layout->addWidget(m_deleteBadge_, 0, Qt::AlignHCenter);
        layout->addWidget(m_sizeLabel_, 0, Qt::AlignHCenter);
        layout->addWidget(m_resolutionLabel_, 0, Qt::AlignHCenter);

        setFixedWidth(settings::ThumbnailWidth + settings::ThumbnailSpacing);
        setFixedHeight(settings::ThumbnailWidth + settings::BadgeHeight + settings::MetaHeight + 20); // 20 = margins + checkbox

        QPixmap pix = OrientImage(m_entry_.path, m_entry_.rotation);
        if (!pix.isNull()) {
            m_thumbnailLabel_->setPixmap(pix.scaled(
                m_thumbnailLabel_->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            ));
        }

        connect(m_checkBox_, &QCheckBox::toggled, this, [this](bool checked) {
            setState(checked ? State::Delete : State::Keep);
            emit selectionChanged(checked);
            });
    }

    void ImageThumbWidget::setState(State state)
    {

        m_state_ = state;
        m_keepBadge_->setVisible(state == State::Keep);
        m_deleteBadge_->setVisible(state == State::Delete);
        m_checkBox_->blockSignals(true);
        m_checkBox_->setChecked(state == State::Delete);
        m_checkBox_->blockSignals(false);
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
