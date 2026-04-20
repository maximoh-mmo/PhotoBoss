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
        m_rootLayout_ = new QVBoxLayout(this);
        m_rootLayout_->setContentsMargins(2, 2, 2, 2);
        m_rootLayout_->setSpacing(6);

        // --- Header ---
        m_header_ = new QLabel(tr("Group (%1 images)").arg(group.images.size()), this);
        m_rootLayout_->addWidget(m_header_);

        // --- Thumbnail rows ---
        m_currentRow_ = new QHBoxLayout();
        m_currentRow_->setSpacing(6);
        m_currentRow_->setAlignment(Qt::AlignLeft);

        m_rootLayout_->addLayout(m_currentRow_);

        updateGroup(group);

        m_rootLayout_->addStretch(); // push content to top
    }

    void GroupWidget::updateGroup(const ImageGroup& group)
    {
        m_header_->setText(tr("Group (%1 images)").arg(group.images.size()));

        // Add any missing thumbs
        for (size_t i = m_thumbs_.size(); i < group.images.size(); ++i) {
            const auto& entry = group.images[i];
            auto* thumb = new ImageThumbWidget(entry, this);
            m_thumbs_.push_back(thumb);

            connect(thumb, &ImageThumbWidget::clicked, this, &GroupWidget::onThumbClicked);
            connect(thumb, &ImageThumbWidget::selectionChanged, this, &GroupWidget::onThumbSelectionChanged);

            if (i > 0 && i % settings::ThumbnailsPerRow == 0) {
                m_currentRow_ = new QHBoxLayout();
                m_currentRow_->setSpacing(6);
                m_currentRow_->setAlignment(Qt::AlignLeft);
                
                // insert right before the stretch spacing
                m_rootLayout_->insertLayout(m_rootLayout_->count() - 1, m_currentRow_);
            }

            m_currentRow_->addWidget(thumb);
        }

        // Update states for all thumbs
        for (size_t i = 0; i < group.images.size(); ++i) {
            if (static_cast<int>(i) == group.bestIndex)
                m_thumbs_[i]->setState(ImageThumbWidget::State::Keep);
            else
            {

            }
                m_thumbs_[i]->setState(ImageThumbWidget::State::Delete);
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

    int GroupWidget::countSelectedForDeletion() const
    {
        int count = 0;
        for (auto* thumb : m_thumbs_) {
            if (thumb->state() == ImageThumbWidget::State::Delete) {
                count++;
            }
        }
        return count;
    }

    QVector<ImageEntry> GroupWidget::imagesMarkedForDeleteEntries() const
    {
        QVector<ImageEntry> result;
        for (auto* thumb : m_thumbs_) {
            if (thumb->state() == ImageThumbWidget::State::Delete) {
                result.push_back(thumb->Image());
            }
        }
        return result;
    }

}