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
        , m_m_filesToDelete_(filesToDelete)
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
    m_warningLabel_ = new QLabel(this);
    m_warningLabel_->setObjectName("warningLabel");
    m_warningLabel_->setText(
        QString(
            "<html><head/><body>"
            "<p><span class=\"warning-text\">⚠️ Confirm Deletion</span></p>"
            "<p>This will move %1 file(s) to your system trash.</p>"
            "<p>This action can be undone by restoring the files from your system's trash folder.</p>"
            "</body></html>"
        ).arg(filesToDelete.size())
    );
    mainLayout->addWidget(m_warningLabel_);

    // Thumbnail grid in scroll area
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(settings::DeleteConfirmDialogScrollAreaMinHeight);

    m_thumbnailContainer_ = new QWidget(scrollArea);
    m_thumbnailLayout_ = new QGridLayout(m_thumbnailContainer_);
    m_thumbnailLayout_->setSpacing(settings::DeleteConfirmDialogLayoutSpacing);

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
        m_thumbnailLayout_->addWidget(label, i / cols, i % cols);
    }

    scrollArea->setWidget(m_thumbnailContainer_);
    mainLayout->addWidget(scrollArea);

    // Confirmation checkbox
    m_confirmCheckBox_ = new QCheckBox(
        "I understand the risks and want to proceed",
        this
    );
    connect(m_confirmCheckBox_, &QCheckBox::toggled, this, &DeleteConfirmDialog::onCheckBoxToggled);
    mainLayout->addWidget(m_confirmCheckBox_);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton_ = new QPushButton("Cancel", this);
    m_cancelButton_->setDefault(true);
    connect(m_cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton_);

    m_deleteButton_ = new QPushButton("Delete", this);
    m_deleteButton_->setObjectName("deleteButton");
    m_deleteButton_->setEnabled(false);
    connect(m_deleteButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_deleteButton_);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void DeleteConfirmDialog::onCheckBoxToggled(bool checked)
{
    m_deleteButton_->setEnabled(checked);
    if (checked) {
        m_deleteButton_->setDefault(true);
    } else {
        m_cancelButton_->setDefault(true);
    }
}

QPixmap DeleteConfirmDialog::loadAndCacheThumbnail(const QString& filePath)
{
    // Check if we already have this thumbnail cached
    if (m_thumbnailCache_.contains(filePath)) {
        return m_thumbnailCache_[filePath];
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
    m_thumbnailCache_.insert(filePath, pixmap);
    return pixmap;
}

} // namespace photoboss
