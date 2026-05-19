#include "ui/ThumbnailManager.h"
#include "ui/GroupWidget.h"
#include "ui/ImageThumbWidget.h"
#include "ui/PreviewPane.h"
#include "util/AppSettings.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace photoboss {

ThumbnailManager::ThumbnailManager(PreviewPane* previewPane, QObject* parent)
    : QObject(parent)
    , m_previewPane_(previewPane)
{
    m_container_ = new QWidget();
    m_layout_ = new QVBoxLayout(m_container_);

    int spacing = m_layout_->spacing();
    int margins = m_layout_->contentsMargins().left() +
        m_layout_->contentsMargins().right();

    m_minimumWidth_ = settings::ThumbnailsPerRow * settings::ThumbnailWidth
        + settings::ThumbnailsPerRow * spacing + margins;

    m_container_->setMinimumWidth(m_minimumWidth_);
    m_container_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_scrollArea_ = new QScrollArea();
    m_scrollArea_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_scrollArea_->setWidgetResizable(true);
    m_scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea_->setWidget(m_container_);
}

QWidget* ThumbnailManager::rootWidget() const
{
    return m_scrollArea_;
}

void ThumbnailManager::processPendingGroup(const ImageGroup& group)
{
    if (m_groupWidgets_.contains(group.id))
        return;

    auto* widget = new GroupWidget(group, m_container_);
    m_layout_->addWidget(widget);
    m_groupWidgets_[group.id] = widget;

    connect(widget, &GroupWidget::selectionChanged, this, &ThumbnailManager::selectionChanged);
    if (group.images.size() > 1)
        m_scanFoundDuplicates_ = true;

    const auto& thumbsByPath = widget->thumbsByPath();
    for (const auto& entry : group.images) {
        auto* thumb = thumbsByPath.value(entry.path);
        if (thumb)
            assignThumbnailToWidget(entry.path, thumb);
    }

    connect(widget, &GroupWidget::previewImage, m_previewPane_, &PreviewPane::showImage);
}

void ThumbnailManager::processUpdatedGroups(const QMap<quint64, ImageGroup>& updatedGroups)
{
    for (auto it = updatedGroups.constBegin(); it != updatedGroups.constEnd(); ++it) {
        quint64 id = it.key();
        const ImageGroup& group = it.value();
        if (!m_groupWidgets_.contains(id))
            continue;

        m_groupWidgets_[id]->updateGroup(group);
        auto* widget = m_groupWidgets_[id];

        for (const auto& entry : group.images) {
            bool alreadyWaiting = false;
            auto waiters = m_thumbnailWaiters_.values(entry.path);
            for (auto* w : waiters) {
                if (w->parentWidget() == widget) {
                    alreadyWaiting = true;
                    break;
                }
            }
            if (!alreadyWaiting) {
                const auto& thumbsByPath = widget->thumbsByPath();
                auto* thumb = thumbsByPath.value(entry.path);
                if (thumb)
                    assignThumbnailToWidget(entry.path, thumb);
            }
        }
    }
}

void ThumbnailManager::distributeThumbnails(const QMap<QString, QPixmap>& thumbnails)
{
    for (auto it = thumbnails.constBegin(); it != thumbnails.constEnd(); ++it) {
        const QString& path = it.key();
        const QPixmap& pix = it.value();
        auto waiters = m_thumbnailWaiters_.values(path);
        for (auto* thumb : waiters)
            thumb->setThumbnail(pix);
        m_thumbnailWaiters_.remove(path);
    }
}

void ThumbnailManager::clearResults()
{
    QLayoutItem* item;
    while ((item = m_layout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_groupWidgets_.clear();
    m_thumbnailWaiters_.clear();
    m_thumbnailCache_.clear();
    m_scanFoundDuplicates_ = false;
}

void ThumbnailManager::assignThumbnailToWidget(const QString& path, ImageThumbWidget* thumb)
{
    if (m_thumbnailCache_.contains(path))
        thumb->setThumbnail(m_thumbnailCache_[path]);
    else
        m_thumbnailWaiters_.insert(path, thumb);
}

} // namespace photoboss
