#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "pipeline/PipelineController.h"
#include "hashing/HashMethod.h"
#include "ui/GroupWidget.h"
#include "util/AppSettings.h"

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
                m_pipeline_controller_->start({ folder, ui_->subfolders->isChecked() });
            }
            });

        connect(m_pipeline_controller_.get(), &PipelineController::status, this, [this](const QString& message) {
            m_status_bar_->showMessage(message);
			});

		connect(m_pipeline_controller_.get(), &PipelineController::progressUpdate, this, &MainWindow::UpdateProgressBar);

        connect(m_pipeline_controller_.get(), &PipelineController::finalGroups,
            this, &MainWindow::onGroupingFinished, Qt::QueuedConnection);
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

    void MainWindow::onGroupingFinished(const std::vector<ImageGroup> groups)
    {
        this->clearResults(); // important for second runs

        for (const auto& group : groups) {
            auto* widget = new GroupWidget(group, m_thumbnail_container_);
            m_thumbnail_layout_->addWidget(widget);
            connect(widget, &GroupWidget::previewImage, m_preview_pane, &PreviewPane::showImage);
        }
    }

    void MainWindow::clearResults()
    {
        QLayoutItem* item;
        while ((item = m_thumbnail_layout_->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }
}