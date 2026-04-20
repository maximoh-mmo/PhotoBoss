#include "ui/DeleteConfirmDialog.h"
#include "util/humanSize.h"
#include "util/AppSettings.h"
#include <QMessageBox>
#include <QDebug>
#include <QLabel>
#include <QPixmap>
#include <QImage>

namespace photoboss {

    DeleteConfirmDialog::DeleteConfirmDialog(
        const QVector<ImageEntry>& filesToDelete,
        QWidget* parent
    )
        : QDialog(parent)
        , filesToDelete_(filesToDelete)
    {
        setWindowTitle("Confirm Deletion");
        setMinimumSize(settings::DeleteConfirmDialogMinWidth, settings::DeleteConfirmDialogMinHeight);
        setModal(true);

        buildUi(filesToDelete);
    }

    DeleteConfirmDialog::~DeleteConfirmDialog()
    {
    }

void DeleteConfirmDialog::buildUi(const QVector<ImageEntry>& filesToDelete)
{
    auto* mainLayout = new QVBoxLayout(this);

    // Warning header
    warningLabel_ = new QLabel(
        QString(
            "<html><head/><body>"
            "<p><span style=\"color:%1;font-size:14px;font-weight:bold;\">"
            "⚠️ Confirm Deletion"
            "</span></p>"
            "<p>This will move %2 file(s) to your system trash.</p>"
            "<p>This action can be undone by restoring the files from your system's trash folder.</p>"
            "</body></html>"
        ).arg(settings::DeleteConfirmDialogWarningColor).arg(filesToDelete.size()),
        this
    );
    // Insert warning label at the beginning
    mainLayout->insertWidget(0, warningLabel_);

    // Thumbnail grid in scroll area - simplified version without interactive widgets
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(settings::DeleteConfirmDialogScrollAreaMinHeight);

    thumbnailContainer_ = new QWidget(scrollArea);
    thumbnailLayout_ = new QGridLayout(thumbnailContainer_);
    thumbnailLayout_->setSpacing(settings->DeleteConfirmDialogLayoutSpacing);

    int cols = settings::DeleteConfirmDialogGridCols;
    for (int i = 0; i < filesToDelete.size(); ++i) {
        const auto& entry = filesToDelete[i];
        
        // Load and scale image for preview with caching
        QPixmap pixmap = loadAndCacheThumbnail(entry.path);
        if (!pixmap.isNull()) {
            auto* label = new QLabel(this);
            label->setPixmap(pixmap);
            label->setAlignment(Qt::AlignCenter);
            label->setFixedSize(settings::DeleteConfirmDialogThumbnailSize, settings::DeleteConfirmDialogThumbnailSize);
            label->setStyleSheet(QString("border: 1px solid %1; border-radius: 4px;").arg(settings::DeleteConfirmDialogBorderColor));
            thumbnailLayout_->addWidget(label, i / cols, i % cols);
        }
        else {
            // Fallback for failed image loads
            auto* label = new QLabel("Failed\nto load", this);
            label->setAlignment(Qt::AlignCenter);
            label->setFixedSize(settings::DeleteConfirmDialogThumbnailSize, settings::DeleteConfirmDialogThumbnailSize);
            label->setStyleSheet(QString("border: 1px solid %1; border-radius: 4px; color: %2;").arg(settings::DeleteConfirmDialogFailedLoadBorderColor).arg(settings::DeleteConfirmDialogFailedLoadTextColor));
            thumbnailLayout_->addWidget(label, i / cols, i % cols);
        }
    }

    scrollArea->setWidget(thumbnailContainer_);
    mainLayout->addWidget(scrollArea);

    // Confirmation checkbox
    confirmCheckBox_ = new QCheckBox(
        "I understand the risks and want to proceed",
        this
    );
    connect(confirmCheckBox_, &QCheckBox::toggled, this, &DeleteConfirmDialog::onCheckBoxToggled);
    mainLayout->addWidget(confirmCheckBox_);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    cancelButton_ = new QPushButton("Cancel", this);
    cancelButton_->setDefault(true);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton_);

    deleteButton_ = new QPushButton("Delete", this);
    deleteButton_->setEnabled(false);
    deleteButton_->setStyleSheet(QString("background-color: %1; color: %2;").arg(settings::DeleteConfirmDialogDeleteButtonBgColor).arg(settings::DeleteConfirmDialogDeleteButtonTextColor));
    connect(deleteButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(deleteButton_);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}
            else {
                // Fallback for failed image loads
                auto* label = new QLabel("Failed\nto load", this);
                label->setAlignment(Qt::AlignCenter);
                label->setFixedSize(settings::DeleteConfirmDialogThumbnailSize, settings::DeleteConfirmDialogThumbnailSize);
                label->setStyleSheet(QString("border: 1px solid %1; border-radius: 4px; color: %2;").arg(settings::DeleteConfirmDialogFailedLoadBorderColor).arg(settings::DeleteConfirmDialogFailedLoadTextColor));
                thumbnailLayout_->addWidget(label, i / cols, i % cols);
            }
        }

        scrollArea->setWidget(thumbnailContainer_);
        mainLayout->addWidget(scrollArea);

        // Confirmation checkbox
        confirmCheckBox_ = new QCheckBox(
            "I understand the risks and want to proceed",
            this
        );
        connect(confirmCheckBox_, &QCheckBox::toggled, this, &DeleteConfirmDialog::onCheckBoxToggled);
        mainLayout->addWidget(confirmCheckBox_);

        // Buttons
        auto* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        cancelButton_ = new QPushButton("Cancel", this);
        cancelButton_->setDefault(true);
        connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addWidget(cancelButton_);

        deleteButton_ = new QPushButton("Delete", this);
        deleteButton_->setEnabled(false);
        deleteButton_->setStyleSheet(QString("background-color: %1; color: %2;").arg(settings::DeleteConfirmDialogDeleteButtonBgColor).arg(settings::DeleteConfirmDialogDeleteButtonTextColor));
        connect(deleteButton_, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(deleteButton_);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);

        scrollArea->setWidget(thumbnailContainer_);
        mainLayout->addWidget(scrollArea);

        // Warning header
        warningLabel_ = new QLabel(
            QString(
                "<html><head/><body>"
                "<p><span style=\"color:%1;font-size:14px;font-weight:bold;\">"
                "⚠️ Confirm Deletion"
                "</span></p>"
                "<p>This will move %2 file(s) to your system trash.</p>"
                "<p>This action can be undone by restoring the files from your system's trash folder.</p>"
                "</body></html>"
            ).arg(settings::DeleteConfirmDialogWarningColor).arg(filesToDelete.size()),
            this
        );
        // Insert warning label at the beginning
        mainLayout->insertWidget(0, warningLabel_);

        // Confirmation checkbox
        confirmCheckBox_ = new QCheckBox(
            "I understand the risks and want to proceed",
            this
        );
        connect(confirmCheckBox_, &QCheckBox::toggled, this, &DeleteConfirmDialog::onCheckBoxToggled);
        mainLayout->addWidget(confirmCheckBox_);

        // Buttons
        auto* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        cancelButton_ = new QPushButton("Cancel", this);
        cancelButton_->setDefault(true);
        connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addWidget(cancelButton_);

        deleteButton_ = new QPushButton("Delete", this);
        deleteButton_->setEnabled(false);
        deleteButton_->setStyleSheet(QString("background-color: %1; color: %2;").arg(settings::DeleteConfirmDialogDeleteButtonBgColor).arg(settings::DeleteConfirmDialogDeleteButtonTextColor));
        connect(deleteButton_, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(deleteButton_);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
    }

void DeleteConfirmDialog::onCheckBoxToggled(bool checked)
{
    deleteButton_->setEnabled(checked);
    if (checked) {
        deleteButton_->setDefault(true);
    } else {
        cancelButton_->setDefault(true);
    }
}

QPixmap DeleteConfirmDialog::loadAndCacheThumbnail(const QString& filePath)
{
    // Check if we already have this thumbnail cached
    if (thumbnailCache_.contains(filePath)) {
        return thumbnailCache_[filePath];
    }
    
    // Load and scale the image
    QImage img(filePath);
    if (img.isNull()) {
        return QPixmap(); // Return null pixmap for failed loads
    }
    
    // Scale to thumbnail size
    QPixmap pixmap = QPixmap::fromImage(img).scaled(
        settings::DeleteConfirmDialogThumbnailSize, settings::DeleteConfirmDialogThumbnailSize, 
        Qt::KeepAspectRatio, Qt::SmoothTransformation
    );
    
    // Cache the thumbnail
    thumbnailCache_.insert(filePath, pixmap);
    return pixmap;
}

} // namespace photoboss