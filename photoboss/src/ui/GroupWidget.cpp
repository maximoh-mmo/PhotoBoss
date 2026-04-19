#pragma once
#include "ui/GroupWidget.h"
#include "ui/ImageThumbWidget.h"
#include "util/AppSettings.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>

namespace photoboss {

    GroupWidget::GroupWidget(const ImageGroup& group, QWidget* parent)
        : QWidget(parent)
    {
        m_rootLayout = new QVBoxLayout(this);
        m_rootLayout->setContentsMargins(2, 2, 2, 2);
        m_rootLayout->setSpacing(6);

        // --- Header ---
        m_header = new QLabel(tr("Group (%1 images)").arg(group.images.size()), this);
        m_rootLayout->addWidget(m_header);

        // --- Thumbnail rows ---
        m_currentRow = new QHBoxLayout();
        m_currentRow->setSpacing(6);
        m_currentRow->setAlignment(Qt::AlignLeft);

        m_rootLayout->addLayout(m_currentRow);

        updateGroup(group);

        m_rootLayout->addStretch(); // push content to top
    }

    void GroupWidget::updateGroup(const ImageGroup& group)
    {
        m_header->setText(tr("Group (%1 images)").arg(group.images.size()));

        // Add any missing thumbs
        for (size_t i = m_thumbs.size(); i < group.images.size(); ++i) {
            const auto& entry = group.images[i];
            auto* thumb = new ImageThumbWidget(entry, this);
            m_thumbs.push_back(thumb);

            connect(thumb, &ImageThumbWidget::clicked, this, &GroupWidget::onThumbClicked);
            connect(thumb, &ImageThumbWidget::selectionChanged, this, &GroupWidget::onThumbSelectionChanged);

            if (i > 0 && i % settings::ThumbnailsPerRow == 0) {
                m_currentRow = new QHBoxLayout();
                m_currentRow->setSpacing(6);
                m_currentRow->setAlignment(Qt::AlignLeft);
                
                // insert right before the stretch spacing
                m_rootLayout->insertLayout(m_rootLayout->count() - 1, m_currentRow);
            }

            m_currentRow->addWidget(thumb);
        }

        // Update states for all thumbs
        for (size_t i = 0; i < group.images.size(); ++i) {
            if (static_cast<int>(i) == group.bestIndex)
                m_thumbs[i]->setState(ImageThumbWidget::State::Keep);
            else
                m_thumbs[i]->setState(ImageThumbWidget::State::Delete);
        }
    }

    void GroupWidget::onThumbClicked(ImageThumbWidget* clicked)
    {
        emit previewImage(clicked->Image());
    }

    void GroupWidget::onThumbSelectionChanged()
    {
        emit selectionChanged();
    }

    QVector<const HashedImageResult*> GroupWidget::imagesMarkedForDelete() const
    {
        QVector<const HashedImageResult*> result;
        for (auto* thumb : m_thumbs) {
            if (thumb->state() == ImageThumbWidget::State::Delete) {
                result.push_back(reinterpret_cast<const HashedImageResult*>(&thumb->Image()));
            }
        }
        return result;
    }

    QVector<ImageEntry> GroupWidget::imagesMarkedForDeleteEntries() const
    {
        QVector<ImageEntry> result;
        for (auto* thumb : m_thumbs) {
            if (thumb->state() == ImageThumbWidget::State::Delete) {
                result.push_back(thumb->Image());
            }
        }
        return result;
    }

}