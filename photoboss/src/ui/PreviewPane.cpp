#include "ui/PreviewPane.h"
#include <QResizeEvent>
#include <QDebug>

namespace photoboss
{
    PreviewPane::PreviewPane(QWidget* parent) : QWidget(parent)
    {
        m_imageLabel_ = new QLabel(this);
        m_imageLabel_->setAlignment(Qt::AlignCenter);
        m_imageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_imageLabel_->setScaledContents(false); // we scale manually

        m_metaLabel_ = new QLabel(this);
        m_metaLabel_->setAlignment(Qt::AlignCenter);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_imageLabel_, 1); // stretch = 1
        layout->addWidget(m_metaLabel_, 0);

     }

    void PreviewPane::showImage(const ImageEntry& entry)
    {
        QImage pix(entry.path);
        pix = OrientImage(std::move(pix), entry.rotation);
        if (pix.isNull()) {
            qWarning() << "PreviewPane: failed to load image:" << entry.path;
            m_imageLabel_->clear();
            m_metaLabel_->clear();
            m_currentImage_ = QImage();
            return;
        }

        m_currentImage_ = pix;

        // Update metadata
        m_metaLabel_->setText(
            QString("%1 x %2 | %3 | %4")
            .arg(entry.resolution.width())
            .arg(entry.resolution.height())
            .arg(humanSize(entry.fileSize))
            .arg(entry.path)
        );

        // Scale & center the image as a pixmap
        if (!m_imageLabel_->size().isEmpty()) {
            m_imageLabel_->setPixmap(
                QPixmap::fromImage(m_currentImage_).scaled(
                    m_imageLabel_->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                )
            );
        }
    }

    void PreviewPane::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        if (!m_currentImage_.isNull() && !m_imageLabel_->size().isEmpty()) {
            // Keep aspect ratio and center the image
            QPixmap scaled = QPixmap::fromImage(m_currentImage_).scaled(
                m_imageLabel_->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );

            m_imageLabel_->setPixmap(scaled);
        }
    }
}
