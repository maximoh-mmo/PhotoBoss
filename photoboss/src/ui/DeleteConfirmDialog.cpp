#include "ui/DeleteConfirmDialog.h"
#include "ui/ImageThumbWidget.h"
#include "util/humanSize.h"
#include <QMessageBox>
#include <QDebug>

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

        // Thumbnail grid in scroll area
        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setMinimumHeight(200);

        thumbnailContainer_ = new QWidget(scrollArea);
        thumbnailLayout_ = new QGridLayout(thumbnailContainer_);
        thumbnailLayout_->setSpacing(6);

        int cols = 4;
        for (int i = 0; i < filesToDelete.size(); ++i) {
            const auto& entry = filesToDelete[i];
            auto* thumb = new ImageThumbWidget(entry, thumbnailContainer_);
            thumbnailLayout_->addWidget(thumb, i / cols, i % cols);
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