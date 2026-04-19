#include "ui/DeleteConfirmDialog.h"
#include "util/humanSize.h"
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
        setMinimumSize(500, 400);
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
                "<p><span style=\"color:#ff6b6b;font-size:14px;font-weight:bold;\">"
                "⚠️ Confirm Deletion"
                "</span></p>"
                "<p>This will move %1 file(s) to your system trash.</p>"
                "<p>This action can be undone by restoring the files from your system's trash folder.</p>"
                "</body></html>"
            ).arg(filesToDelete.size()),
            this
        );
        mainLayout->addWidget(warningLabel_);

        // Thumbnail grid in scroll area - simplified version without interactive widgets
        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setMinimumHeight(200);

        thumbnailContainer_ = new QWidget(scrollArea);
        thumbnailLayout_ = new QGridLayout(thumbnailContainer_);
        thumbnailLayout_->setSpacing(6);

        int cols = 4;
        for (int i = 0; i < filesToDelete.size(); ++i) {
            const auto& entry = filesToDelete[i];
            
            // Load and scale image for preview
            QImage img(entry.path);
            if (!img.isNull()) {
                // Scale to thumbnail size (similar to preview pane)
                QPixmap pixmap = QPixmap::fromImage(img).scaled(
                    100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation
                );
                
                auto* label = new QLabel(this);
                label->setPixmap(pixmap);
                label->setAlignment(Qt::AlignCenter);
                label->setFixedSize(100, 100);
                label->setStyleSheet("border: 1px solid #555; border-radius: 4px;");
                thumbnailLayout_->addWidget(label, i / cols, i % cols);
            } else {
                // Fallback for failed image loads
                auto* label = new QLabel("Failed\nto load", this);
                label->setAlignment(Qt::AlignCenter);
                label->setFixedSize(100, 100);
                label->setStyleSheet("border: 1px solid #555; border-radius: 4px; color: #888;");
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
        deleteButton_->setStyleSheet("background-color: #c0392b; color: white;");
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

} // namespace photoboss