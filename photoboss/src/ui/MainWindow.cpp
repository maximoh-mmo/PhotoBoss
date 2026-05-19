#include "ui/MainWindow.h"
#include "ui_mainwindow.h"
#include "util/AppSettings.h"
#include "ui/GroupWidget.h"
#include "ui/PreviewPane.h"
#include "ui/ProgressCounterWidget.h"
#include "ui/ThumbnailManager.h"
#include "ui/DeletionService.h"
#include "ui/UiUpdateQueue.h"
#include "pipeline/PipelineController.h"
#include "pipeline/Pipeline.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>

namespace photoboss {

MainWindow::MainWindow(std::unique_ptr<PipelineController> controller,
                       std::unique_ptr<ThumbnailManager> thumbnailManager,
                       std::unique_ptr<PreviewPane> previewPane,
                       std::unique_ptr<DeletionService> deletionService,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_ui_(new Ui::MainWindow)
    , m_pipeline_controller_(std::move(controller))
    , m_thumbnailManager_(std::move(thumbnailManager))
    , m_preview_pane_(std::move(previewPane))
    , m_deletionService_(std::move(deletionService))
{
    m_ui_->setupUi(this);
    init();
}

MainWindow::~MainWindow()
{
    m_pipeline_controller_->stop();
    delete m_ui_;
}

void MainWindow::init()
{
    m_status_bar_ = m_ui_->statusbar;
    m_browse_button_ = m_ui_->browsebutton;
    m_scan_button_ = m_ui_->scan;
    m_progress_bar_ = m_ui_->progressBar;
    m_btn_delete_ = m_ui_->btnDelete;
    m_delete_count_label_ = m_ui_->deleteCountLabel;

    // Phase indicator labels
    m_phase_indicators_[Pipeline::Phase::Find] = new ProgressCounterWidget("Scanning", this);
    m_phase_indicators_[Pipeline::Phase::Analyze] = new ProgressCounterWidget("Analysis", this);
    m_phase_indicators_[Pipeline::Phase::Group] = new ProgressCounterWidget("Grouping", this);

    QWidget* container = new QWidget(m_ui_->phaseStrip);
    QHBoxLayout* layout = new QHBoxLayout(container);
    for (auto& indicator : m_phase_indicators_) {
        layout->addWidget(indicator);
    }
    layout->setAlignment(Qt::AlignRight);
    layout->setContentsMargins(0, 0, 0, 0);
    m_ui_->phaseStripLayout->addWidget(container);

    // Split body area: thumbnails | preview
    m_splitter_ = new QSplitter(Qt::Horizontal);
    m_splitter_->addWidget(m_thumbnailManager_->rootWidget());
    m_splitter_->addWidget(m_preview_pane_.get());

    m_preview_pane_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_preview_pane_->setMinimumWidth(m_thumbnailManager_->minimumWidthHint());

    m_splitter_->setStretchFactor(0, 0);
    m_splitter_->setStretchFactor(1, 1);
    m_splitter_->setSizes({ m_thumbnailManager_->minimumWidthHint(), 1 });

    QVBoxLayout* bodyLayout = new QVBoxLayout(m_ui_->body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->addWidget(m_splitter_);

    m_btn_delete_->setVisible(false);
    m_delete_count_label_->setVisible(false);

    m_deletionService_->setDialogParent(this);

    connect(m_thumbnailManager_.get(), &ThumbnailManager::selectionChanged,
            this, &MainWindow::onGroupSelectionChanged);
    connect(m_deletionService_.get(), &DeletionService::deletionCompleted,
            this, [this]() {
                clearResults();
                updateDeleteButtonState();
            });

    wireConnections();
}

void MainWindow::onBrowse()
{
    if (QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath()); !directory.isEmpty())
    {
        setCurrentFolder(directory);
    }
}

void MainWindow::wireConnections()
{
    connect(m_browse_button_, &QPushButton::clicked, this, &MainWindow::onBrowse);
    connect(m_scan_button_, &QPushButton::clicked, this, [this]() {
        auto state = m_pipeline_controller_->state();
        if (state == Pipeline::PipelineState::Running) {
            m_pipeline_controller_->stop();
            for (auto* widget : m_phase_indicators_) {
                widget->stop();
            }
        }
        else if (state == Pipeline::PipelineState::Stopped) {
            const QString folder = getCurrentFolder();
            if (!folder.isEmpty()) {
                m_pipeline_controller_->uiQueue()->reset();
                clearResults();
                for (auto* widget : m_phase_indicators_) {
                    widget->prepareForScan();
                }
                m_pipeline_controller_->start({ folder, m_ui_->subfolders->isChecked() });
            }
        }
    });

    connect(m_pipeline_controller_->uiQueue(), &UiUpdateQueue::snapshotReady,
            this, &MainWindow::applySnapshot, Qt::QueuedConnection);

    connect(m_btn_delete_, &QPushButton::clicked,
            m_deletionService_.get(), &DeletionService::executeDeletion);

    connect(m_ui_->actionQuit, &QAction::triggered, this, &QWidget::close);
    connect(m_ui_->actionLicense, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("License"),
            tr("PhotoBoss is provided under the MIT License.\n\n"
               "See the LICENSE file for details."));
    });
}

void MainWindow::onCurrentFolderChanged()
{
    m_ui_->filepath->setPlainText(m_current_folder_);
    m_pipeline_controller_->uiQueue()->reset();
    clearResults();
}

void MainWindow::setCurrentFolder(const QString& folder)
{
    if (folder != m_current_folder_) {
        m_current_folder_ = folder;
        onCurrentFolderChanged();
    }
}

void MainWindow::applySnapshot(const UiSnapshot& snap)
{
    m_lastSnapshot_ = snap;

    int processed = 0;
    while (!snap.pendingGroups.empty() && processed < settings::MainWindowBatchProcessSize) {
        m_thumbnailManager_->processPendingGroup(snap.pendingGroups.front());
        ++processed;
    }
    if (processed > 0) {
        m_pipeline_controller_->uiQueue()->commitProcessed(processed);
    }

    m_thumbnailManager_->processUpdatedGroups(snap.updatedGroups);
    m_thumbnailManager_->distributeThumbnails(snap.thumbnailCache);

    updatePhaseProgress(snap);

    int curSelection = m_deletionService_->countSelected();
    if (snap.pipelineState != m_lastPipelineState_ || curSelection != m_lastSelectionCount_) {
        onPipelineStateChanged(snap.pipelineState);
        updateDeleteButtonState();
        m_lastSelectionCount_ = curSelection;
        m_lastPipelineState_ = snap.pipelineState;
    }
}

void MainWindow::updatePhaseProgress(const UiSnapshot& snap)
{
    int cumCur = 0, cumTot = 0;

    if (snap.phaseProgress.isEmpty()) {
        cumTot = 1;
    }

    if (snap.phaseProgress.isEmpty() &&
        ((m_lastPipelineState_ == Pipeline::PipelineState::Stopped && snap.pipelineState == Pipeline::PipelineState::Stopped) ||
         (m_lastPipelineState_ == Pipeline::PipelineState::Running && snap.pipelineState == Pipeline::PipelineState::Stopped))) {
        for (auto* widget : m_phase_indicators_.values()) {
            widget->reset();
        }
    }

    for (auto it = snap.phaseProgress.constBegin(); it != snap.phaseProgress.constEnd(); ++it) {
        if (m_phase_indicators_.contains(it.key())) {
            if (it.key() == Pipeline::Phase::Find) {
                cumTot = 3 * it.value().second;
            }
            cumCur += it.value().first;
            m_phase_indicators_[it.key()]->setTotal(it.value().second);
            m_phase_indicators_[it.key()]->setProgress(it.value().first);
        }
    }

    m_status_bar_->showMessage(snap.statusMessage);

    if (m_progress_bar_) {
        m_progress_bar_->setMaximum(cumTot);
        m_progress_bar_->setValue(cumCur);
    }
}

void MainWindow::clearResults()
{
    m_thumbnailManager_->clearResults();
    m_preview_pane_->clear();
}

void MainWindow::onPipelineStateChanged(Pipeline::PipelineState state)
{
    switch (state) {
    case Pipeline::PipelineState::Running:
        m_scan_button_->setText(tr("Stop Scan"));
        m_scan_button_->setEnabled(true);
        m_browse_button_->setEnabled(false);
        m_btn_delete_->setVisible(false);
        break;

    case Pipeline::PipelineState::Stopping:
        m_scan_button_->setText(tr("Stopping..."));
        m_scan_button_->setEnabled(false);
        m_browse_button_->setEnabled(false);
        break;

    case Pipeline::PipelineState::Stopped:
        m_scan_button_->setText(tr("Start Scan"));
        m_scan_button_->setEnabled(true);
        m_browse_button_->setEnabled(true);

        if (m_thumbnailManager_->foundDuplicates()) {
            m_status_bar_->showMessage(
                QString("%1 Duplicates found -- Scan Complete").arg(m_thumbnailManager_->groupCount()));
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
    int count = m_deletionService_->countSelected();

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
        bool canDelete = count > 0 && m_thumbnailManager_->foundDuplicates()
            && m_pipeline_controller_->state() == Pipeline::PipelineState::Stopped;
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

void MainWindow::onGroupSelectionChanged()
{
    updateDeleteButtonState();
}

} // namespace photoboss
