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
        QImage pix(entry.path);
        pix = OrientImage(std::move(pix), entry.rotation);
        if (pix.isNull()) {
            qWarning() << "PreviewPane: failed to load image:" << entry.path;
            m_image_label_->clear();
            m_meta_label_->clear();
            m_current_image_ = QImage();
            return;
        }

        m_current_image_ = pix;

        // Update metadata
        m_meta_label_->setText(
            QString("%1 x %2 | %3 | %4")
            .arg(entry.resolution.width())
            .arg(entry.resolution.height())
            .arg(humanSize(entry.fileSize))
            .arg(entry.path)
        );

        // Scale & center the image as a pixmap
        if (!m_image_label_->size().isEmpty()) {
            m_image_label_->setPixmap(
                QPixmap::fromImage(m_current_image_).scaled(
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

        if (!m_current_image_.isNull() && !m_image_label_->size().isEmpty()) {
            // Keep aspect ratio and center the image
            QPixmap scaled = QPixmap::fromImage(m_current_image_).scaled(
                m_image_label_->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );

            m_image_label_->setPixmap(scaled);
        }
    }
}
