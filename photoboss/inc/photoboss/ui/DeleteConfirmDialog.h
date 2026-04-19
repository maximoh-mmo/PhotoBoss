#pragma once

#include <QDialog>
#include <QVector>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "util/GroupTypes.h"

namespace photoboss {

    class DeleteConfirmDialog : public QDialog {
        Q_OBJECT

    public:
        explicit DeleteConfirmDialog(
            const QVector<ImageEntry>& filesToDelete,
            QWidget* parent = nullptr
        );
        ~DeleteConfirmDialog();

        QVector<ImageEntry> filesToDelete() const { return filesToDelete_; }

    private slots:
        void onCheckBoxToggled(bool checked);

    private:
        void buildUi(const QVector<ImageEntry>& filesToDelete);

        QVector<ImageEntry> filesToDelete_;

        QLabel* warningLabel_ = nullptr;
        QWidget* thumbnailContainer_ = nullptr;
        QGridLayout* thumbnailLayout_ = nullptr;
        QCheckBox* confirmCheckBox_ = nullptr;
        QPushButton* cancelButton_ = nullptr;
        QPushButton* deleteButton_ = nullptr;
    };

} // namespace photoboss