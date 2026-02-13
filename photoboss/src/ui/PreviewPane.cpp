#include "ui/PreviewPane.h"
#include <QResizeEvent>
#include <QDebug>

namespace photoboss
{
    PreviewPane::PreviewPane(QWidget* parent) : QWidget(parent)
    {
        m_image_label_ = new QLabel(this);
        m_image_label_->setAlignment(Qt::AlignCenter);
        m_image_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_image_label_->setScaledContents(false); // we scale manually

        m_meta_label_ = new QLabel(this);
        m_meta_label_->setAlignment(Qt::AlignCenter);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_image_label_, 1); // stretch = 1
        layout->addWidget(m_meta_label_, 0);

     }

    void PreviewPane::showImage(const ImageEntry& entry)
    {
        QPixmap pix = OrientImage(entry.path, entry.rotation);
        if (pix.isNull()) {
            qWarning() << "PreviewPane: failed to load image:" << entry.path;
            m_image_label_->clear();
            m_meta_label_->clear();
            m_current_pixmap_ = QPixmap();
            return;
        }

        m_current_pixmap_ = pix;

        // Update metadata
        m_meta_label_->setText(
            QString("%1 x %2 | %3")
            .arg(entry.resolution.width())
            .arg(entry.resolution.height())
            .arg(humanSize(entry.fileSize))
        );

        // Scale & center the pixmap
        if (!m_image_label_->size().isEmpty()) {
            m_image_label_->setPixmap(
                m_current_pixmap_.scaled(
                    m_image_label_->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                )
            );
        }
    }

    void PreviewPane::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        if (!m_current_pixmap_.isNull() && !m_image_label_->size().isEmpty()) {
            // Keep aspect ratio and center the image
            QPixmap scaled = m_current_pixmap_.scaled(
                m_image_label_->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );

            m_image_label_->setPixmap(scaled);
        }
    }
}
