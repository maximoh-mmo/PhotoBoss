#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "pipeline/PipelineController.h"
#include "pipeline/PipelineFactory.h"
#include "hashing/HashMethod.h"
#include "ui/GroupWidget.h"
#include "util/AppSettings.h"
#include "ui/ImageThumbWidget.h"
#include "ui/DeleteConfirmDialog.h"
#include "ui/ProgressCounterWidget.h"
#include "ui/UiStatusModel.h"
#include "util/StorageInfo.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

namespace photoboss
{
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), m_ui_(new Ui::MainWindow)

    {
        m_pipeline_controller_ = std::make_unique<PipelineController>(this);
		m_pipeline_factory_ = std::make_unique<PipelineFactory>(this);

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
        m_factory_scan_button_ = m_ui_->factoryScan;
        m_progress_bar_ = m_ui_->progressBar;
        m_btn_delete_ = m_ui_->btnDelete;
        m_delete_count_label_ = m_ui_->deleteCountLabel;

        // Phase indicator labels
        m_phase_indicators_[PipelineController::Phase::Find] = new ProgressCounterWidget("Scan Progress", this);
        m_phase_indicators_[PipelineController::Phase::Analyze] = new ProgressCounterWidget("Analyse Progress", this);
        m_phase_indicators_[PipelineController::Phase::Group] = new ProgressCounterWidget("Group Progress", this);
		QWidget* container = new QWidget(m_ui_->phaseStrip);
        QHBoxLayout* layout = new QHBoxLayout(container);
        for (auto& indicator : m_phase_indicators_) {
			layout->addWidget(indicator);
		}
		m_ui_->phaseStripLayout->addWidget(container);

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

        // Model that holds UI‑state for polling
        m_statusModel_ = std::make_unique<UiStatusModel>(this);

        // UI poll timer – 30 fps
        m_uiPollTimer_ = new QTimer(this);
        m_uiPollTimer_->setInterval(settings::UiPollIntervalMs);
        connect(m_uiPollTimer_, &QTimer::timeout, this, &MainWindow::processUiUpdates);
        // The original batch timer is retained for possible legacy use but will no longer be started.
        m_batchTimer_ = new QTimer(this);
        m_batchTimer_->setInterval(settings::MainWindowBatchTimerInterval); // 50fps-ish
        // No connection – processing is now done in processUiUpdates
        // We start the poll timer in onPipelineStateChanged when Running.
        // The old onGroupAdded logic that started the batch timer is removed.


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
            }
            else if (state == PipelineController::PipelineState::Stopped) {
                const QString folder = GetCurrentFolder();
                if (!folder.isEmpty()) {
                    clearResults();
                    m_pipeline_controller_->start({ folder, m_ui_->subfolders->isChecked() });
                }
            }
            // If Stopping, ignore clicks (button is disabled)
            });

        connect(m_factory_scan_button_, &QPushButton::clicked, this, [this]() {
            auto state = m_pipeline_controller_->state();
            for (const auto& spinner : m_phase_indicators_) {
                spinner->switchState();
            }
            });

        // Forward status messages to the model (no UI side‑effect here)
        connect(m_pipeline_controller_.get(), &PipelineController::status, this,
            [this](const QString& message){ m_statusModel_->setStatusMessage(message); }, Qt::QueuedConnection);

        // Forward updates to the model for polling
        connect(m_pipeline_controller_.get(), &PipelineController::groupAdded,
            this, [this](const ImageGroup& g){ m_statusModel_->addPendingGroup(g); }, Qt::QueuedConnection);
        connect(m_pipeline_controller_.get(), &PipelineController::groupUpdated,
            this, [this](const ImageGroup& g){ m_statusModel_->updateGroup(g); }, Qt::QueuedConnection);
        connect(m_pipeline_controller_.get(), &PipelineController::thumbnailReady,
            this, [this](const ThumbnailResult& r){ m_statusModel_->setThumbnail(r); }, Qt::QueuedConnection);
        connect(m_pipeline_controller_.get(), &PipelineController::phaseUpdate,
            this, [this](PipelineController::Phase p, int c, int t){ m_statusModel_->setPhaseProgress(p, c, t); }, Qt::QueuedConnection);
        // pipelineStateChanged still handled directly (button enable/disable)
        connect(m_pipeline_controller_.get(), &PipelineController::pipelineStateChanged,
            this, &MainWindow::onPipelineStateChanged, Qt::QueuedConnection);

        connect(m_btn_delete_, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);

        // Show status messages (still needed for user feedback)
        connect(m_pipeline_controller_.get(), &PipelineController::status, this,
            [this](const QString& message){ m_status_bar_->showMessage(message); });

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
        // Deprecated: original batch processing is now handled by polling.
        // Kept for compatibility; does nothing.
    }

    // New polling‑driven UI update method
    void MainWindow::processUiUpdates()
    {
        // 1️⃣ Get current snapshot
        UiStatusModel::Snapshot snap = m_statusModel_->snapshot();

        // 2️⃣ Early‑out if nothing changed since last poll
        if (snap == m_lastSnapshot_)
            return; // no UI work required
        // Keep a copy for the next iteration
        m_lastSnapshot_ = snap;

        // 3️⃣ Process pending groups (batch limited)
        int processed = 0;
        while (!snap.pendingGroups.empty() && processed < settings::MainWindowBatchProcessSize) {
            ImageGroup group = snap.pendingGroups.front();
            snap.pendingGroups.pop_front(); // local copy only
            if (m_groupWidgets_.contains(group.id)) { ++processed; continue; }

            auto* widget = new GroupWidget(group, m_thumbnail_container_);
            m_thumbnail_layout_->addWidget(widget);
            m_groupWidgets_[group.id] = widget;
            connect(widget, &GroupWidget::selectionChanged, this, &MainWindow::onGroupSelectionChanged);
            if (group.images.size() > 1) m_scan_found_duplicates_ = true;
            for (const auto& entry : group.images) {
                for (auto* thumb : widget->findChildren<ImageThumbWidget*>()) {
                    if (thumb->Image().path == entry.path) {
                        if (m_thumbnailCache_.contains(entry.path))
                            thumb->setThumbnail(m_thumbnailCache_[entry.path]);
                        else
                            m_thumbnailWaiters_.insert(entry.path, thumb);
                    }
                }
            }
            connect(widget, &GroupWidget::previewImage, m_preview_pane_, &PreviewPane::showImage);
            ++processed;
        }

        // 4️⃣ Updated groups
        for (auto it = snap.updatedGroups.constBegin(); it != snap.updatedGroups.constEnd(); ++it) {
            quint64 id = it.key();
            const ImageGroup& group = it.value();
            if (m_groupWidgets_.contains(id)) {
                m_groupWidgets_[id]->updateGroup(group);
                auto* widget = m_groupWidgets_[id];
                for (const auto& entry : group.images) {
                    bool alreadyWaiting = false;
                    auto waiters = m_thumbnailWaiters_.values(entry.path);
                    for (auto* w : waiters) {
                        if (w->parentWidget() == widget) { alreadyWaiting = true; break; }
                    }
                    if (!alreadyWaiting) {
                        for (auto* thumb : widget->findChildren<ImageThumbWidget*>()) {
                            if (thumb->Image().path == entry.path) {
                                if (m_thumbnailCache_.contains(entry.path))
                                    thumb->setThumbnail(m_thumbnailCache_[entry.path]);
                                else
                                    m_thumbnailWaiters_.insert(entry.path, thumb);
                            }
                        }
                    }
                }
            }
        }

        // 5️⃣ Thumbnails
        for (auto it = snap.thumbnailCache.constBegin(); it != snap.thumbnailCache.constEnd(); ++it) {
            const QString& path = it.key();
            const QPixmap& pix = it.value();
            auto waiters = m_thumbnailWaiters_.values(path);
            for (auto* thumb : waiters) thumb->setThumbnail(pix);
            m_thumbnailWaiters_.remove(path);
        }

        // 6️⃣ Phase progress
        int cumCur = 0, cumTot = 0;
        for (auto it = snap.phaseProgress.constBegin(); it != snap.phaseProgress.constEnd(); ++it) {
            if (m_phase_indicators_.contains(it.key())) {
                if (it.key() == PipelineController::Phase::Find) {
                    cumTot = 3 * it.value().second;
				}
				cumCur += it.value().first;
                m_phase_indicators_[it.key()]->setProgress(it.value().first);
                m_phase_indicators_[it.key()]->setTotal(it.value().second);
            }
        }

        // 7️⃣ Status bar
        m_status_bar_->showMessage(snap.statusMessage);

        // 8️⃣ Cumulative progress bar
        if (m_progress_bar_) {
            m_progress_bar_->setMaximum(cumTot);
            m_progress_bar_->setValue(cumCur);
        }

        // 9️⃣ Delete button – update only when state or selection changed
        static PipelineController::PipelineState lastState = PipelineController::PipelineState::Stopped;
        int curSelection = countSelectedForDeletion();
        if (snap.pipelineState != lastState || curSelection != m_lastSelectionCount_) {
            updateDeleteButtonState();
            m_lastSelectionCount_ = curSelection;
            lastState = snap.pipelineState;
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
                            }
                            else {
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
            // start UI polling
            if (m_uiPollTimer_ && !m_uiPollTimer_->isActive())
                m_uiPollTimer_->start();
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
            // Stop UI polling when not running
            if (m_uiPollTimer_ && m_uiPollTimer_->isActive())
                m_uiPollTimer_->stop();
            // Leave progress bar at 100% to show completion

            if (m_scan_found_duplicates_) {
                m_status_bar_->showMessage(
                    QString("%1 Duplicates found -- Scan Complete").arg(m_groupWidgets_.size()));
            }
            else {
                m_status_bar_->showMessage("No duplicates found -- Scan Complete");
            }

            updateDeleteButtonState();
            break;
        }
    }

    void MainWindow::updateDeleteButtonState()
    {
        int count = countSelectedForDeletion();
       
        if (m_delete_count_label_) {
            if (count > 0) {
                m_delete_count_label_->setText(QString("%1 file(s) marked for deletion").arg(count));
                m_delete_count_label_->setVisible(true);
            }
            else {
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
            }
            else {
                m_btn_delete_->setVisible(false);
                m_btn_delete_->setEnabled(false);
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
            }
            else {
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
        for (auto& indicator : m_phase_indicators_) {
            indicator->reset();
        }
    }
}