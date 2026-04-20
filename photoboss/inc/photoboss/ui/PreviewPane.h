#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include "util/humanSize.h"
#include "util/OrientImage.h"
#include "util/GroupTypes.h"

namespace photoboss
{
    class PreviewPane : public QWidget
    {
        Q_OBJECT
    public:
        explicit PreviewPane(QWidget* parent = nullptr);

        void showImage(const ImageEntry& entry);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private:
        QLabel* m_imageLabel_;
        QLabel* m_metaLabel_;
        QImage m_currentImage_;
    };
}
