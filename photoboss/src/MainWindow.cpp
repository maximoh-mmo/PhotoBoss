#include "mainwindow.h"
#include "ui_mainwindow.h"  // This gets auto-generated from the .ui
#include "settingsSelection.h"
#include <QFileDialog>
#include "HashWorker.h"
#include "ImageScanner.h"
#include "HashMethod.h"
#include "ResultProcessor.h"

namespace photoboss
{
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent)
        , ui_(new Ui::MainWindow)
        , m_scanner_(new ImageScanner())
        , m_disk_reader_(new DiskReader(m_disk_read_queue_))
        , m_file_list(new std::list<ImageFileMetaData>)
        , m_hash_methods()
        , m_result_processor_(new ResultProcessor(m_resultQueue_))
    {
        ui_->setupUi(this);
        Init();
    }

    MainWindow::~MainWindow() {
        m_disk_read_queue_.shutdown();
        // Signal workers to stop if possible
        if (m_scanner_) m_scanner_->Cancel();
        if (m_disk_reader_) m_disk_reader_->Cancel();
        for (auto worker : m_hash_workers_) {
            if (worker)
            {
                worker->Cancel();
                delete worker;
            }
        }

        // Quit and wait for threads
        if (m_scanner_thread_) {
            m_scanner_thread_->quit();
            m_scanner_thread_->wait();
            delete m_scanner_thread_;
        }
        if (m_reader_thread_) {
            m_reader_thread_->quit();
            m_reader_thread_->wait();
            delete m_reader_thread_;
        }
        for (auto thread : m_hash_worker_threads_)
        {
            if (thread)
            {
                thread->quit();
                thread->wait();
                delete thread;
            }
        }

        // Delete workers
        delete m_scanner_;
        delete m_disk_reader_;
        delete ui_;
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

    void MainWindow::OnFilePathsCollected(const std::list<ImageFileMetaData>& meta_data)
    {
        m_file_list = std::make_unique<std::list<ImageFileMetaData>>(std::move(meta_data));
        m_disk_reader_->Start(m_file_list);
    }

    void MainWindow::WireConnections()
    {
        connect(m_scanner_, &ImageScanner::scanned_file_count, this, &MainWindow::OnScannedFileCount);
        connect(m_browse_button_, &QPushButton::clicked, this, &MainWindow::OnBrowse);
        connect(m_scan_button_, &QPushButton::clicked, this, [this]() {
            const QString folder = GetCurrentFolder();
            const bool recursive = ui_->subfolders->checkState() == Qt::Checked;
            QMetaObject::invokeMethod(
                m_scanner_,
                "StartScan",
                Qt::QueuedConnection,
                Q_ARG(QString, folder),
                Q_ARG(bool, recursive)
            );
            });
        connect(m_disk_reader_, &DiskReader::ReadProgress, this, &MainWindow::UpdateDiskReadProgress);
        connect(m_scanner_, &ImageScanner::filePathsCollected, this, &MainWindow::OnFilePathsCollected); 
		connect(ui_->actionSettings, &QAction::triggered, this, &MainWindow::openSettings);
    }

    void MainWindow::Init()
    {
        m_scanner_thread_ = new QThread(this);
        m_reader_thread_ = new QThread(this);
        m_result_thread_ = new QThread(this);
        m_file_dialog_ = new QFileDialog(this);
        m_status_bar_ = ui_->statusbar;
        m_browse_button_ = ui_->browsebutton;
        m_scan_button_ = ui_->scan;
        m_progress_bar_ = ui_->progressBar;

        m_file_dialog_->setFileMode(QFileDialog::Directory);
        m_file_dialog_->setOption(QFileDialog::ShowDirsOnly, true);
        
        // Create HashMethod instances from the registry
        for (auto& hashMethods : HashRegistry::createAll()) {
            m_hash_methods.emplace_back(std::shared_ptr<HashMethod>(std::move(hashMethods)));
        }   
        
        WireConnections();

        // Set up additional threads
        m_disk_reader_->moveToThread(m_reader_thread_);
        m_reader_thread_->start();
        m_scanner_->moveToThread(m_scanner_thread_);
        m_scanner_thread_->start();
        m_result_processor_->moveToThread(m_result_thread_);
        m_result_thread_->start();

        // Create HashWorker threads
        int threadCount = QThread::idealThreadCount();
        for (int i = 0; i < threadCount; ++i) {
            auto thread = new QThread(this);
            auto worker = new HashWorker(m_disk_read_queue_, m_hash_methods);

            m_hash_worker_threads_.push_back(thread);
            m_hash_workers_.push_back(worker);

            worker->moveToThread(thread);
            connect(thread, &QThread::started, worker, &HashWorker::run);
            connect(worker, &HashWorker::image_hashed, this, [this](std::shared_ptr<HashedImageResult> result) {
                qDebug() << "Hashed:" << result->meta.path << "->" << result->hashes;
				m_resultQueue_.push(std::move(result));
                });

            thread->start();
        }

        connect(m_result_thread_, &QThread::started, m_result_processor_, &ResultProcessor::run);

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

    void MainWindow::OnGroupFound(Group group)
    {
        // Handle the group found event, e.g., update UI or store the group
        // This is a placeholder for actual logic to handle the found group.
        qDebug() << "Group found: " << group.name << " at path: " << group.path;
    }

    void MainWindow::SetCurrentFolder(const QString& folder)
    {
        if (folder != m_current_folder_) {
            m_current_folder_ = folder;
            OnCurrentFolderChanged();
        }
    }

    void MainWindow::openSettings() {
        auto settingsDialog = new SettingsSelection(m_hash_methods, this);

        connect(settingsDialog, &SettingsSelection::settingsApplied,
            this, [this](const std::vector<std::shared_ptr<HashMethod>>& methods) {
                m_hash_methods = methods;
                // Optionally, do other things with the new settings
            });

        settingsDialog->exec(); // modal
        settingsDialog->deleteLater(); // clean up after closing
    }
}