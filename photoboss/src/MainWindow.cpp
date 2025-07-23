#include "mainwindow.h"
#include "ui_mainwindow.h"  // This gets auto-generated from the .ui
#include <QFileDialog>

#include "HashWorker.h"
#include "ImageScanner.h"

namespace photoboss
{
	MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui_(new Ui::MainWindow)
        , m_scanner_(new ImageScanner())
		, m_disk_reader_(new DiskReader(m_disk_read_queue_))
        , m_file_list(new std::list<ImageFileMetaData>)
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
    }
    
    void MainWindow::Init()
    {
        m_scanner_thread_ = new QThread(this);
		m_reader_thread_ = new QThread(this);
        m_file_dialog_ = new QFileDialog(this);
		m_status_bar_ = ui_->statusbar;
        m_browse_button_ = ui_->browsebutton;
        m_scan_button_ = ui_->scan;
        m_progress_bar_ = ui_->progressBar;
    
        m_file_dialog_->setFileMode(QFileDialog::Directory);
        m_file_dialog_->setOption(QFileDialog::ShowDirsOnly, true);

		WireConnections();

        // Set up additional threads
		m_disk_reader_->moveToThread(m_reader_thread_);
		m_reader_thread_->start();
        m_scanner_->moveToThread(m_scanner_thread_);
        m_scanner_thread_->start();

        int threadCount = QThread::idealThreadCount();
        for (int i = 0; i < threadCount; ++i) {
            auto thread = new QThread(this);
            auto worker = new HashWorker(m_disk_read_queue_);

            m_hash_worker_threads_.push_back(thread);
            m_hash_workers_.push_back(worker);

            worker->moveToThread(thread);
            connect(thread, &QThread::started, worker, &HashWorker::run);
            connect(worker, &HashWorker::image_hashed, this, [](HashedImageResult* result) {
            qDebug() << "Hashed:" << result->meta.path << "->" << result->hash;
            delete result;  // Clean up the result after processing
            });

            thread->start();
        }
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
}
    