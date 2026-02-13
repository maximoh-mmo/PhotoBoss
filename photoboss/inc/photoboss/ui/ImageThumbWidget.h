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
            Keep,
            Delete
        };
        Q_ENUM(State)

        explicit ImageThumbWidget(const ImageEntry entry, QWidget* parent = nullptr);

        void setState(State state);
        State state() const { return m_state_; }
        
        const ImageEntry& Image() { return m_entry_; }

    signals:
        void clicked(ImageThumbWidget*);
        void selectionChanged(bool selected);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        void buildUi();

        ImageEntry m_entry_;
        State m_state_ = State::Keep;

        QCheckBox* m_checkBox_ = nullptr;
        QLabel* m_thumbnailLabel_ = nullptr;
        QLabel* m_sizeLabel_ = nullptr;
        QLabel* m_resolutionLabel_ = nullptr;
        QLabel* m_deleteBadge_ = nullptr;
        QLabel* m_keepBadge_ = nullptr;
    };
}