#pragma once
#include <QWidget>
#include <qlabel.h>
#include <qscrollarea.h>
#include "util/GroupTypes.h"

namespace photoboss {

    class PreviewPane : public QWidget
    {
        Q_OBJECT
    public:
        explicit PreviewPane(QWidget* parent = nullptr);

    public slots:
        void showImage(const ImageEntry& entry);

    private:
        QLabel* m_image_label_;
        QLabel* m_meta_label_;
        QScrollArea* m_scrollArea;
    };
}
