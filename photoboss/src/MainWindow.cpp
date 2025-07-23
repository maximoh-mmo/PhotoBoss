#include "mainwindow.h"
#include "ui_mainwindow.h"  // This gets auto-generated from the .ui
#include <QFileDialog>
#include "ImageScanner.h"

namespace photoboss
{
	MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui_(new Ui::MainWindow)
        , m_scanner_(new ImageScanner())
		, m_disk_reader_(new DiskReader)
    {
        ui_->setupUi(this);
        Init();
    }
    
    MainWindow::~MainWindow() {
		m_scanner_thread_->quit();
		m_reader_thread_->quit();
        m_scanner_thread_->wait();
        m_reader_thread_->wait();
		delete m_reader_thread_;
        delete m_scanner_thread_;
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

    void MainWindow::OnImageReady(const std::unique_ptr<DiskReadResult>& result)
    {
    }

    void MainWindow::WireConnections()
    {
        connect(m_scanner_thread_, &QThread::finished, m_scanner_, &QObject::deleteLater);
        connect(m_scanner_, &ImageScanner::scanned_file_count, this, &MainWindow::OnScannedFileCount);
        connect(m_scanner_thread_, &QThread::finished, m_scanner_, &QObject::deleteLater);
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
        connect(m_disk_reader_, &DiskReader::ImageRead, this, &MainWindow::OnImageReady);
        connect(m_disk_reader_, &DiskReader::ReadProgress, this, &MainWindow::UpdateDiskReadProgress);
        connect(m_disk_reader_, &DiskReader::Finished, m_disk_reader_, &QObject::deleteLater);
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
    