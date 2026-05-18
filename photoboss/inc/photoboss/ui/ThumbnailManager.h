#pragma once

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QPixmap>
#include <deque>

#include "types/GroupTypes.h"
#include "types/DataTypes.h"

class QScrollArea;
class QVBoxLayout;
class QWidget;

namespace photoboss {

class GroupWidget;
class ImageThumbWidget;
class PreviewPane;

class ThumbnailManager : public QObject {
    Q_OBJECT

public:
    explicit ThumbnailManager(PreviewPane* previewPane, QObject* parent = nullptr);

    void processPendingGroup(const ImageGroup& group);
    void processUpdatedGroups(const QMap<quint64, ImageGroup>& updatedGroups);
    void distributeThumbnails(const QMap<QString, QPixmap>& thumbnails);

    void clearResults();

    bool foundDuplicates() const { return m_scanFoundDuplicates_; }
    int groupCount() const { return m_groupWidgets_.size(); }
    const QMap<quint64, GroupWidget*>& groupWidgets() const { return m_groupWidgets_; }

    QWidget* rootWidget() const;
    int minimumWidthHint() const { return m_minimumWidth_; }

signals:
    void selectionChanged();

private:
    void assignThumbnailToWidget(const QString& path, ImageThumbWidget* thumb);

    PreviewPane* m_previewPane_;
    QScrollArea* m_scrollArea_;
    QWidget* m_container_;
    QVBoxLayout* m_layout_;
    int m_minimumWidth_ = 0;

    QMap<quint64, GroupWidget*> m_groupWidgets_;
    QMultiMap<QString, ImageThumbWidget*> m_thumbnailWaiters_;
    QMap<QString, QPixmap> m_thumbnailCache_;
    bool m_scanFoundDuplicates_ = false;
};

} // namespace photoboss
