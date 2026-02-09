#pragma once
#include <QWidget.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include "util/GroupTypes.h"

namespace photoboss {

    class ImageThumbWidget : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(State state READ state WRITE setState)

    public:
        enum class State {
            Normal,
            Selected,
            MarkedForDelete
        };
        Q_ENUM(State)

        explicit ImageThumbWidget(QWidget* parent = nullptr);
        ~ImageThumbWidget() override = default;

        // State
        void setState(State state);
        State state() const { return m_state_; }
        
        // Content
        void setImage(const ImageEntry* entry);
        void setThumbnail(const QPixmap& pixmap);
        void setMeta(QString size, QString resolution);

    signals:
        void clicked(ImageThumbWidget*);
        void selectionChanged(bool selected);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        void buildUi();
        void updateVisualState();

        const ImageEntry* m_entry_ = nullptr;
        State m_state_ = State::Normal;

        QCheckBox* m_checkBox_ = nullptr;
        QLabel* m_thumbnailLabel_ = nullptr;
        QLabel* m_sizeLabel_ = nullptr;
        QLabel* m_resolutionLabel_ = nullptr;
        QLabel* m_deleteBadge_ = nullptr;
        QLabel* m_keepBadge_ = nullptr;
    };
}