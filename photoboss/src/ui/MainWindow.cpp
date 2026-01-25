#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/SettingsSelection.h"
#include "pipeline/PipelineController.h"
#include "hashing/HashMethod.h"

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

        WireConnections();

        // Connect pipeline signals
        connect(m_pipeline_controller_.get(), &PipelineController::imageHashed,
			this, &MainWindow::OnImageHashed);

        m_pipeline_controller_->start();
    }
    void MainWindow::OnBrowse()
    {
        if (QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath()); !directory.isEmpty())
        {
            SetCurrentFolder(directory);
        }
    }

    void MainWindow::UpdateDiskReadProgress(int current, int total)
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
                m_pipeline_controller_->startScan(folder, ui_->subfolders->isChecked());
            }
            });

        connect(ui_->actionSettings, &QAction::triggered, this, &MainWindow::openSettings);
        connect(m_pipeline_controller_.get(), &PipelineController::status, this, [this](const QString& message) {
            m_status_bar_->showMessage(message);
			});
    }

    void MainWindow::OnCurrentFolderChanged()
    {
        ui_->filepath->setPlainText(m_current_folder_);
    }

    void MainWindow::OnScannedFileCount(int fileCount)
    {
        m_progress_bar_->setMaximum(fileCount);
        m_progress_bar_->setValue(0);
        m_status_bar_->showMessage(tr("%1 Files Scanned...").arg(fileCount));
    }

    void MainWindow::SetCurrentFolder(const QString& folder)
    {
        if (folder != m_current_folder_) {
            m_current_folder_ = folder;
            OnCurrentFolderChanged();
        }
    }

    void MainWindow::openSettings()
    {
        SettingsSelection settingsDialog(this);
        connect(&settingsDialog, &SettingsSelection::settingsApplied,
            this, [this](const std::set<QString>& hashes) {
                m_pipeline_controller_->updateActiveHashes(hashes);
            });
        settingsDialog.exec();
    }

    void MainWindow::OnImageHashed(std::shared_ptr<HashedImageResult> result)
    {
        // Update progress bar
        m_progress_bar_->setValue(m_progress_bar_->value() + 1);

        if (result) {
            auto it = result->hashes.find("Perceptual Hash");
            if (it != result->hashes.end()) {
                qDebug() << "Hashed:" << it->second;
            }
            else {
                qDebug() << "Hashed: <not found>";
            }
        }
    }
}