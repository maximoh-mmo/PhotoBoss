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
#include <QMap>
#include <QPixmap>
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

        QVector<ImageEntry> filesToDelete() const { return m_filesToDelete_; }

    private slots:
        void onCheckBoxToggled(bool checked);

    private:
        void buildUi(const QVector<ImageEntry>& filesToDelete);
        QPixmap loadAndCacheThumbnail(const QString& filePath);

        QVector<ImageEntry> m_filesToDelete_;
        QMap<QString, QPixmap> m_thumbnailCache_;

        QLabel* m_warningLabel_ = nullptr;
        QWidget* m_thumbnailContainer_ = nullptr;
        QGridLayout* m_thumbnailLayout_ = nullptr;
        QCheckBox* m_confirmCheckBox_ = nullptr;
        QPushButton* m_cancelButton_ = nullptr;
        QPushButton* m_deleteButton_ = nullptr;
    };

} // namespace photoboss