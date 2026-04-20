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
        setObjectName("DeleteConfirmDialog");
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
    warningLabel_ = new QLabel(this);
    warningLabel_->setObjectName("warningLabel");
    warningLabel_->setText(
        QString(
            "<html><head/><body>"
            "<p><span class=\"warning-text\">⚠️ Confirm Deletion</span></p>"
            "<p>This will move %1 file(s) to your system trash.</p>"
            "<p>This action can be undone by restoring the files from your system's trash folder.</p>"
            "</body></html>"
        ).arg(filesToDelete.size())
    );
    mainLayout->addWidget(warningLabel_);

    // Thumbnail grid in scroll area
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(settings::DeleteConfirmDialogScrollAreaMinHeight);

    thumbnailContainer_ = new QWidget(scrollArea);
    thumbnailLayout_ = new QGridLayout(thumbnailContainer_);
    thumbnailLayout_->setSpacing(settings::DeleteConfirmDialogLayoutSpacing);

    int cols = settings::DeleteConfirmDialogGridCols;
    for (int i = 0; i < filesToDelete.size(); ++i) {
        const auto& entry = filesToDelete[i];
        
        // Load and scale image for preview with caching
        QPixmap pixmap = loadAndCacheThumbnail(entry.path);
        
        auto* label = new QLabel(this);
        label->setObjectName("thumbnailLabel");
        label->setAlignment(Qt::AlignCenter);
        label->setFixedSize(settings::DeleteConfirmDialogThumbnailSize, settings::DeleteConfirmDialogThumbnailSize);
        
        if (!pixmap.isNull()) {
            label->setPixmap(pixmap);
        } else {
            label->setText("Failed\nto load");
            label->setObjectName("thumbnailLabelFailed");
        }
        thumbnailLayout_->addWidget(label, i / cols, i % cols);
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
    deleteButton_->setObjectName("deleteButton");
    deleteButton_->setEnabled(false);
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
