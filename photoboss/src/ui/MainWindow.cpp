#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "pipeline/PipelineController.h"
#include "hashing/HashMethod.h"
#include "ui/GroupWidget.h"
#include "util/AppSettings.h"
#include "ui/ImageThumbWidget.h"

#include <QFileDialog>
#include <QDebug>

namespace photoboss
{
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui_(new Ui::MainWindow)

    {
        m_pipeline_controller_ = std::make_unique<PipelineController>(this);

        ui_->setupUi(this);

        Init();
    }

    MainWindow::~MainWindow()
    {
        if (m_pipeline_controller_) {
            m_pipeline_controller_->stop();
        }
        delete ui_;
    }
    void MainWindow::Init()
    {
        m_status_bar_ = ui_->statusbar;
        m_browse_button_ = ui_->browsebutton;
        m_scan_button_ = ui_->scan;
        m_progress_bar_ = ui_->progressBar;

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
        m_preview_pane = new PreviewPane();

        m_splitter_->addWidget(m_preview_pane);
        m_splitter_->setStretchFactor(0, 0);
        m_splitter_->setStretchFactor(1, 1);
        m_splitter_->setSizes({totalWidth,1});

        QVBoxLayout* bodyLayout = new QVBoxLayout(ui_->body);
        bodyLayout->setContentsMargins(0, 0, 0, 0);
        bodyLayout->addWidget(m_splitter_);

        m_batchTimer = new QTimer(this);
        m_batchTimer->setInterval(20); // 50fps-ish
        connect(m_batchTimer, &QTimer::timeout, this, &MainWindow::processBatch);
        // We start it in onGroupAdded as needed

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
            const QString folder = GetCurrentFolder();
            if (!folder.isEmpty()) {
                clearResults(); // Clear UI for new scan
                m_pipeline_controller_->start({ folder, ui_->subfolders->isChecked() });
            }
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
    }

    void MainWindow::OnCurrentFolderChanged()
    {
        ui_->filepath->setPlainText(m_current_folder_);
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
        if (m_pendingGroups.empty()) return;

        int count = 0;
        while (!m_pendingGroups.empty() && count < 10) { // increased batch size
            ImageGroup group = m_pendingGroups.front();
            m_pendingGroups.pop_front();

            if (m_groupWidgets.contains(group.id)) continue;

            auto* widget = new GroupWidget(group, m_thumbnail_container_);
            m_thumbnail_layout_->addWidget(widget);
            m_groupWidgets[group.id] = widget;

            // Register thumbnails for async update and check cache
            for (const auto& entry : group.images) {
                // Find the ImageThumbWidget inside the GroupWidget
                for (auto* thumb : widget->findChildren<ImageThumbWidget*>()) {
                    if (thumb->Image().path == entry.path) {
                        if (m_thumbnailCache.contains(entry.path)) {
                            thumb->setThumbnail(m_thumbnailCache[entry.path]);
                        } else {
                            m_thumbnailWaiters.insert(entry.path, thumb);
                        }
                    }
                }
            }

            connect(widget, &GroupWidget::previewImage, m_preview_pane, &PreviewPane::showImage);
            count++;
        }
    }

    void MainWindow::onGroupAdded(const ImageGroup& group)
    {
        m_pendingGroups.push_back(group);
        if (!m_batchTimer->isActive()) {
            m_batchTimer->start(50); // Process batch every 50ms
        }
    }

    void MainWindow::onGroupUpdated(const ImageGroup& group)
    {
        if (m_groupWidgets.contains(group.id)) {
            m_groupWidgets[group.id]->updateGroup(group);

            // Also update mapping for any new images added to the group
            auto* widget = m_groupWidgets[group.id];
            for (const auto& entry : group.images) {
                bool alreadyWaiting = false;
                auto waiters = m_thumbnailWaiters.values(entry.path);
                for (auto* w : waiters) {
                    if (w->parentWidget()->parentWidget() == widget) { // Very hacky path to GroupWidget
                       alreadyWaiting = true;
                       break;
                    }
                }
                
                if (!alreadyWaiting) {
                    for (auto* thumb : widget->findChildren<ImageThumbWidget*>()) {
                        if (thumb->Image().path == entry.path) {
                             if (m_thumbnailCache.contains(entry.path)) {
                                 thumb->setThumbnail(m_thumbnailCache[entry.path]);
                             } else {
                                 m_thumbnailWaiters.insert(entry.path, thumb);
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
        m_thumbnailCache.insert(result.path, pix);

        auto waiters = m_thumbnailWaiters.values(result.path);
        for (auto* thumb : waiters) {
            thumb->setThumbnail(pix);
        }
        
        m_thumbnailWaiters.remove(result.path);
    }

    void MainWindow::clearResults()
    {
        QLayoutItem* item;
        while ((item = m_thumbnail_layout_->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        m_groupWidgets.clear();
        m_pendingGroups.clear();
        m_thumbnailWaiters.clear();
        m_thumbnailCache.clear();
    }
}