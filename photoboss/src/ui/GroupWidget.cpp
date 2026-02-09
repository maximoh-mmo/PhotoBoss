#include "ui/GroupWidget.h"
#include "ui/ImageThumbWidget.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>

namespace photoboss {

    GroupWidget::GroupWidget(const ImageGroup& group, QWidget* parent)
        : QWidget(parent)
    {
        auto* root = new QVBoxLayout(this);

        auto* header = new QLabel(
            tr("Group (%1 images)").arg(group.images.size()), this);
        root->addWidget(header);

        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        for (size_t i = 0; i < group.images.size(); ++i) {
            const auto& entry = group.images[i];

            auto* thumb = new ImageThumbWidget(this);
            thumb->setImage(&entry);

            if (static_cast<int>(i) == group.bestIndex)
                thumb->setState(ImageThumbWidget::State::Selected);
            else
                thumb->setState(ImageThumbWidget::State::MarkedForDelete);

            connect(
                thumb,
                &ImageThumbWidget::clicked,
                this,
                &GroupWidget::onThumbClicked
            );

            m_thumbs.push_back(thumb);
            row->addWidget(thumb);
        }

        row->addStretch();
        root->addLayout(row);
    }

    void GroupWidget::onThumbClicked(ImageThumbWidget* clicked)
    {
        for (auto* thumb : m_thumbs) {
            thumb->setState(
                thumb == clicked
                ? ImageThumbWidget::State::Selected
                : ImageThumbWidget::State::Normal);
        }
    }

}