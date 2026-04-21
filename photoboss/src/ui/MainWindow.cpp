#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "pipeline/PipelineController.h"
#include "hashing/HashMethod.h"
#include "ui/GroupWidget.h"
#include "util/AppSettings.h"
#include "ui/ImageThumbWidget.h"
#include "ui/DeleteConfirmDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

namespace photoboss
{
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), m_ui_(new Ui::MainWindow)

    {
        m_pipeline_controller_ = std::make_unique<PipelineController>(this);

        m_ui_->setupUi(this);

        Init();
    }

    MainWindow::~MainWindow()
    {
        if (m_pipeline_controller_) {
            m_pipeline_controller_->stop();
        }
        delete m_ui_;
    }
    void MainWindow::Init()
    {
        m_status_bar_ = m_ui_->statusbar;
        m_browse_button_ = m_ui_->browsebutton;
        m_scan_button_ = m_ui_->scan;
        m_progress_bar_ = m_ui_->progressBar;
        m_btn_delete_ = m_ui_->btnDelete;
        m_delete_count_label_ = m_ui_->deleteCountLabel;

        // Phase indicator labels
        m_phase_finding_label_ = m_ui_->phaseFindingLabel;
        m_phase_finding_count_ = m_ui_->phaseFindingCount;
        m_phase_analyzing_label_ = m_ui_->phaseAnalyzingLabel;
        m_phase_analyzing_count_ = m_ui_->phaseAnalyzingCount;
        m_phase_grouping_label_ = m_ui_->phaseGroupingLabel;
        m_phase_grouping_count_ = m_ui_->phaseGroupingCount;

        // Split body area
        m_splitter_ = new QSplitter(Qt::Horizontal);

        // LHS thumbnail container
        m_thumbnail_container_ = new QWidget();
        m_thumbnail_layout_ = new QVBoxLayout(m_thumbnail_container_);
        int spacing = m_thumbnail_layout_->spacing();
        int margins = m_thumbnail_layout_->contentsMargins().left() +
            m_thumbnail_layout_->contentsMargins().right();

        int totalWidth = settings::ThumbnailsPerRow * settings::ThumbnailWidth
            + (settings::ThumbnailsPerRow - 1) * spacing + margins;

        m_thumbnail_container_->setMinimumWidth(totalWidth);
        m_thumbnail_container_->setMaximumWidth(totalWidth);
        m_thumbnail_container_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        // Wrap thumbnails in scroll area
        m_thumbnail_scroll_ = new QScrollArea();
        m_thumbnail_scroll_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        m_thumbnail_scroll_->setWidgetResizable(true);
        m_thumbnail_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_thumbnail_scroll_->setWidget(m_thumbnail_container_);

        m_splitter_->addWidget(m_thumbnail_scroll_);

        // Preview pane
        m_preview_pane_ = new PreviewPane();

        m_splitter_->addWidget(m_preview_pane_);
        m_splitter_->setStretchFactor(0, 0);
        m_splitter_->setStretchFactor(1, 1);
        m_splitter_->setSizes({ totalWidth,1 });

        QVBoxLayout* bodyLayout = new QVBoxLayout(m_ui_->body);
        bodyLayout->setContentsMargins(0, 0, 0, 0);
        bodyLayout->addWidget(m_splitter_);

        m_batchTimer_ = new QTimer(this);
        m_batchTimer_->setInterval(settings::MainWindowBatchTimerInterval); // 50fps-ish
        connect(m_batchTimer_, &QTimer::timeout, this, &MainWindow::processBatch);
        // We start it in onGroupAdded as needed

        m_btn_delete_->setVisible(false);
        m_delete_count_label_->setVisible(false);
        WireConnections();
    }

    void MainWindow::OnBrowse()
    {
        if (QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath()); !directory.isEmpty())
        {
            SetCurrentFolder(directory);
        }
    }

    void MainWindow::UpdateProgressBar(int current, int total)
    {
        if (m_progress_bar_) {
            m_progress_bar_->setMaximum(total);
            m_progress_bar_->setValue(current);
        }
    }

    void MainWindow::WireConnections()
    {
        connect(m_browse_button_, &QPushButton::clicked, this, &MainWindow::OnBrowse);
        connect(m_scan_button_, &QPushButton::clicked, this, [this]() {
            auto state = m_pipeline_controller_->state();
            if (state == PipelineController::PipelineState::Running) {
                m_pipeline_controller_->stop();
            } else if (state == PipelineController::PipelineState::Stopped) {
                const QString folder = GetCurrentFolder();
                if (!folder.isEmpty()) {
                    clearResults();
                    m_pipeline_controller_->start({ folder, m_ui_->subfolders->isChecked() });
                }
            }
            // If Stopping, ignore clicks (button is disabled)
        });

        connect(m_pipeline_controller_.get(), &PipelineController::status, this, [this](const QString& message) {
            m_status_bar_->showMessage(message);
			});

		connect(m_pipeline_controller_.get(), &PipelineController::progressUpdate, this, &MainWindow::UpdateProgressBar);

        connect(m_pipeline_controller_.get(), &PipelineController::groupAdded,
            this, &MainWindow::onGroupAdded, Qt::QueuedConnection);

        connect(m_pipeline_controller_.get(), &PipelineController::groupUpdated,
            this, &MainWindow::onGroupUpdated, Qt::QueuedConnection);

        connect(m_pipeline_controller_.get(), &PipelineController::thumbnailReady,
            this, &MainWindow::onThumbnailReady, Qt::QueuedConnection);

        connect(m_pipeline_controller_.get(), &PipelineController::pipelineStateChanged,
            this, &MainWindow::onPipelineStateChanged, Qt::QueuedConnection);

        connect(m_btn_delete_, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);

// Status bar - just display messages
        connect(m_pipeline_controller_.get(), &PipelineController::status, this,
            [this](const QString& message) {
                m_status_bar_->showMessage(message);
            });

        // Phase-specific signals - direct connections, no parsing
        connect(m_pipeline_controller_.get(), &PipelineController::phaseFindingUpdate,
            this, &MainWindow::updatePhaseFinding);

        connect(m_pipeline_controller_.get(), &PipelineController::phaseAnalyzingUpdate,
            this, &MainWindow::updatePhaseAnalyzing);

        connect(m_pipeline_controller_.get(), &PipelineController::phaseGroupingUpdate,
            this, &MainWindow::updatePhaseGrouping);
    }

    void MainWindow::OnCurrentFolderChanged()
    {
        m_ui_->filepath->setPlainText(m_current_folder_);
    }

    void MainWindow::SetCurrentFolder(const QString& folder)
    {
        if (folder != m_current_folder_) {
            m_current_folder_ = folder;
            OnCurrentFolderChanged();
        }
    }

void MainWindow::processBatch()
{
    if (m_pendingGroups_.empty()) return;

    int count = 0;
    while (!m_pendingGroups_.empty() && count < settings::MainWindowBatchProcessSize) {
            ImageGroup group = m_pendingGroups_.front();
            m_pendingGroups_.pop_front();

            if (m_groupWidgets_.contains(group.id)) continue;

            auto* widget = new GroupWidget(group, m_thumbnail_container_);
            m_thumbnail_layout_->addWidget(widget);
            m_groupWidgets_[group.id] = widget;

            // Connect selection changes to update delete button state
            connect(widget, &GroupWidget::selectionChanged, this, &MainWindow::onGroupSelectionChanged);

            // Track if we found duplicates (groups with >1 image)
            if (group.images.size() > 1) {
                m_scan_found_duplicates_ = true;
            }

            // Register thumbnails for async update and check cache
            for (const auto& entry : group.images) {
                // Find the ImageThumbWidget inside the GroupWidget
                for (auto* thumb : widget->findChildren<ImageThumbWidget*>()) {
                    if (thumb->Image().path == entry.path) {
                        if (m_thumbnailCache_.contains(entry.path)) {
                            thumb->setThumbnail(m_thumbnailCache_[entry.path]);
                        } else {
                            m_thumbnailWaiters_.insert(entry.path, thumb);
                        }
                    }
                }
            }

            connect(widget, &GroupWidget::previewImage, m_preview_pane_, &PreviewPane::showImage);
            count++;
        }
    }

    void MainWindow::onGroupAdded(const ImageGroup& group)
    {
        m_pendingGroups_.push_back(group);
        if (!m_batchTimer_->isActive()) {
            m_batchTimer_->start(50); // Process batch every 50ms
        }
    }

    void MainWindow::onGroupUpdated(const ImageGroup& group)
    {
        if (m_groupWidgets_.contains(group.id)) {
            m_groupWidgets_[group.id]->updateGroup(group);

             // Also update mapping for any new images added to the group
             auto* widget = m_groupWidgets_[group.id];
             for (const auto& entry : group.images) {
                 bool alreadyWaiting = false;
                 auto waiters = m_thumbnailWaiters_.values(entry.path);
                 for (auto* w : waiters) {
                     if (w->parentWidget() == widget) {
                        alreadyWaiting = true;
                        break;
                     }
                 }
                
                if (!alreadyWaiting) {
                    for (auto* thumb : widget->findChildren<ImageThumbWidget*>()) {
                        if (thumb->Image().path == entry.path) {
                             if (m_thumbnailCache_.contains(entry.path)) {
                                 thumb->setThumbnail(m_thumbnailCache_[entry.path]);
                             } else {
                                 m_thumbnailWaiters_.insert(entry.path, thumb);
                             }
                        }
                    }
                }
            }
        }
    }

    void MainWindow::onThumbnailReady(const ThumbnailResult& result)
    {
        QPixmap pix = QPixmap::fromImage(result.image);
        m_thumbnailCache_.insert(result.path, pix);

        auto waiters = m_thumbnailWaiters_.values(result.path);
        for (auto* thumb : waiters) {
            thumb->setThumbnail(pix);
        }
        
        m_thumbnailWaiters_.remove(result.path);
    }

    void MainWindow::clearResults()
    {
        QLayoutItem* item;
        while ((item = m_thumbnail_layout_->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        m_groupWidgets_.clear();
        m_pendingGroups_.clear();
        m_thumbnailWaiters_.clear();
        m_thumbnailCache_.clear();
        m_scan_found_duplicates_ = false;
    }

    void MainWindow::onPipelineStateChanged(PipelineController::PipelineState state)
    {
        switch (state) {
        case PipelineController::PipelineState::Running:
            m_scan_button_->setText(tr("Stop Scan"));
            m_scan_button_->setEnabled(true);
            m_browse_button_->setEnabled(false);
            m_btn_delete_->setVisible(false);
            resetPhaseIndicators();
            break;

        case PipelineController::PipelineState::Stopping:
            m_scan_button_->setText(tr("Stopping..."));
            m_scan_button_->setEnabled(false);
            m_browse_button_->setEnabled(false);
            break;

        case PipelineController::PipelineState::Stopped:
            m_scan_button_->setText(tr("Start Scan"));
            m_scan_button_->setEnabled(true);
            m_browse_button_->setEnabled(true);
            // Leave progress bar at 100% to show completion

            // Mark phases complete and show final status
            setPhaseComplete("finding");
            setPhaseComplete("analyzing");
            setPhaseComplete("grouping");

            if (m_scan_found_duplicates_) {
                m_status_bar_->showMessage(
                    QString("%1 Duplicates found -- Scan Complete").arg(m_groupWidgets_.size()));
            } else {
                m_status_bar_->showMessage("No duplicates found -- Scan Complete");
            }

            updateDeleteButtonState();
            break;
        }
    }

    void MainWindow::updateDeleteButtonState()
    {
        int count = countSelectedForDeletion();
        qDebug() << "[Delete Button] Count:" << count
                 << "Duplicates found:" << m_scan_found_duplicates_
                 << "Group count:" << m_groupWidgets_.size()
                 << "Pipeline state:" << static_cast<int>(m_pipeline_controller_->state());

        if (m_delete_count_label_) {
            if (count > 0) {
                m_delete_count_label_->setText(QString("%1 file(s) marked for deletion").arg(count));
                m_delete_count_label_->setVisible(true);
            } else {
                m_delete_count_label_->setVisible(false);
            }
        }

        if (m_btn_delete_) {
            // Only enable delete button when pipeline is fully stopped
            bool canDelete = count > 0 && m_scan_found_duplicates_
                         && m_pipeline_controller_->state() == PipelineController::PipelineState::Stopped;
            if (canDelete) {
                m_btn_delete_->setVisible(true);
                m_btn_delete_->setEnabled(true);
                qDebug() << "[Delete Button] Setting visible and enabled";
            } else {
                m_btn_delete_->setVisible(false);
                m_btn_delete_->setEnabled(false);
                qDebug() << "[Delete Button] Setting hidden/disabled";
            }
        }
    }

    int MainWindow::countSelectedForDeletion() const
    {
        int count = 0;
        for (auto* widget : m_groupWidgets_.values()) {
            count += widget->countSelectedForDeletion();
        }
        return count;
    }

    QVector<ImageEntry> MainWindow::collectSelectedForDeletion() const
    {
        QVector<ImageEntry> result;
        for (auto* widget : m_groupWidgets_.values()) {
            const auto& marked = widget->imagesMarkedForDeleteEntries();
            for (const auto& img : marked) {
                result.push_back(img);
            }
        }
        return result;
    }

    void MainWindow::onDeleteClicked()
    {
        auto files = collectSelectedForDeletion();
        if (files.isEmpty()) return;

        DeleteConfirmDialog dialog(files, this);
        if (dialog.exec() == QDialog::Accepted) {
            bool allDeleted = true;
            QStringList failedFiles;
            
            for (const auto& file : files) {
                if (!QFile::moveToTrash(file.path)) {
                    allDeleted = false;
                    failedFiles.append(file.path);
                }
            }
            
            if (allDeleted) {
                QMessageBox::information(
                    this,
                    "Delete Successful",
                    QString("Successfully moved %1 file(s) to trash.").arg(files.size())
                );
            } else {
                QMessageBox::warning(
                    this,
                    "Partial Failure",
                    QString("Failed to move %1 file(s) to trash:\n%2")
                        .arg(failedFiles.size())
                        .arg(failedFiles.join("\n"))
                );
            }

            // Refresh UI after deletion
            clearResults();
            updateDeleteButtonState();
        }
    }

    void MainWindow::onGroupSelectionChanged()
    {
        updateDeleteButtonState();
    }

    void MainWindow::resetPhaseIndicators()
    {
        if (m_phase_finding_count_) {
            m_phase_finding_count_->setText("—");
            m_phase_finding_count_->setStyleSheet("color: gray;");
            m_phase_finding_label_->setStyleSheet("color: gray;");
        }
        if (m_phase_analyzing_count_) {
            m_phase_analyzing_count_->setText("—");
            m_phase_analyzing_count_->setStyleSheet("color: gray;");
            m_phase_analyzing_label_->setStyleSheet("color: gray;");
        }
        if (m_phase_grouping_count_) {
            m_phase_grouping_count_->setText("—");
            m_phase_grouping_count_->setStyleSheet("color: gray;");
            m_phase_grouping_label_->setStyleSheet("color: gray;");
        }
    }

    void MainWindow::updatePhaseFinding(int count)
    {
        if (m_phase_finding_count_) {
            m_phase_finding_count_->setText(QString::number(count));
            m_phase_finding_count_->setStyleSheet("color: #F59E0B; font-weight: bold;");
            m_phase_finding_label_->setStyleSheet("color: #F59E0B; font-weight: bold;");
        }
    }

    void MainWindow::updatePhaseAnalyzing(int count)
    {
        if (m_phase_analyzing_count_) {
            m_phase_analyzing_count_->setText(QString::number(count));
            m_phase_analyzing_count_->setStyleSheet("color: #F59E0B; font-weight: bold;");
            m_phase_analyzing_label_->setStyleSheet("color: #F59E0B; font-weight: bold;");
        }
    }

    void MainWindow::updatePhaseGrouping(int count)
    {
        if (m_phase_grouping_count_) {
            m_phase_grouping_count_->setText(QString::number(count));
            m_phase_grouping_count_->setStyleSheet("color: #F59E0B; font-weight: bold;");
            m_phase_grouping_label_->setStyleSheet("color: #F59E0B; font-weight: bold;");
        }
    }

    void MainWindow::setPhaseComplete(const QString& phase)
    {
        if (phase == "finding") {
            if (m_phase_finding_count_) {
                m_phase_finding_count_->setStyleSheet("color: #10B981; font-weight: bold;");
                m_phase_finding_label_->setStyleSheet("color: #10B981; font-weight: bold;");
            }
        } else if (phase == "analyzing") {
            if (m_phase_analyzing_count_) {
                m_phase_analyzing_count_->setStyleSheet("color: #10B981; font-weight: bold;");
                m_phase_analyzing_label_->setStyleSheet("color: #10B981; font-weight: bold;");
            }
        } else if (phase == "grouping") {
            if (m_phase_grouping_count_) {
                m_phase_grouping_count_->setStyleSheet("color: #10B981; font-weight: bold;");
                m_phase_grouping_label_->setStyleSheet("color: #10B981; font-weight: bold;");
            }
        }
    }
}
