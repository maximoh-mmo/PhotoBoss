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
        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(2, 2, 2, 2);
        rootLayout->setSpacing(6);

        // --- Header ---
        auto* header = new QLabel(tr("Group (%1 images)").arg(group.images.size()), this);
        rootLayout->addWidget(header);

        // --- Thumbnail rows ---
        auto currentRow = new QHBoxLayout();
        currentRow->setSpacing(6);
        currentRow->setAlignment(Qt::AlignLeft);

        rootLayout->addLayout(currentRow);

        for (size_t i = 0; i < group.images.size(); ++i) {
            const auto& entry = group.images[i];
            auto* thumb = new ImageThumbWidget(std::move(entry), this);
            m_thumbs.push_back(thumb);

            // Determine state
            if (static_cast<int>(i) == group.bestIndex)
                thumb->setState(ImageThumbWidget::State::Keep);
            else
                thumb->setState(ImageThumbWidget::State::Delete);

            connect(thumb, &ImageThumbWidget::clicked, this, &GroupWidget::onThumbClicked);

            // Start new row if needed
            if (i % settings::ThumbnailsPerRow == 0) {
                currentRow = new QHBoxLayout();
                currentRow->setSpacing(6);
                currentRow->setAlignment(Qt::AlignLeft); // left-align incomplete rows
                rootLayout->addLayout(currentRow);
            }

            currentRow->addWidget(thumb);
        }

        rootLayout->addStretch(); // push content to top
    }

    void GroupWidget::onThumbClicked(ImageThumbWidget* clicked)
    {
        emit previewImage(clicked->Image());
    }

}