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
    {
        ui_->setupUi(this);
        init();
    }
    
    MainWindow::~MainWindow() {
        m_scanner_thread_->quit();
        m_scanner_thread_->wait();
        delete m_scanner_thread_;
        delete ui_;
    }
    
    void MainWindow::on_browse()
    {
        QString directory = m_file_dialog_->getExistingDirectory(this, tr("Open Directory"), QDir::homePath());
        if (!directory.isEmpty()) {
            set_current_folder(directory);
        }
    }

    void MainWindow::wire_connections()
    {
        connect(m_scanner_thread_, &QThread::finished, m_scanner_, &QObject::deleteLater);
        connect(m_scanner_, &ImageScanner::fileCount, this, &MainWindow::on_scanned_file_count);
        connect(m_scanner_thread_, &QThread::finished, m_scanner_, &QObject::deleteLater);
        connect(m_browse_button_, &QPushButton::clicked, this, &MainWindow::on_browse);
        connect(m_scan_button_, &QPushButton::clicked, this, [this]() {
            m_scanner_->StartScan(get_current_folder(), ui_->subfolders->checkState());
            });
    }
    
    void MainWindow::init()
    {
        m_scanner_thread_ = new QThread(this);
        m_status_bar_ = ui_->statusbar;
        m_browse_button_ = ui_->browsebutton;
        m_scan_button_ = ui_->scan;
        m_progress_bar_ = ui_->progressBar;
    
        wire_connections();
    
        m_scanner_->moveToThread(m_scanner_thread_);
        m_scanner_thread_->start();
    
        m_file_dialog_ = new QFileDialog(this);
        m_file_dialog_->setFileMode(QFileDialog::Directory);
        m_file_dialog_->setOption(QFileDialog::ShowDirsOnly, true);
    
    }
    
    void MainWindow::on_current_folder_changed()
    {
        ui_->filepath->setPlainText(m_current_folder_);
    }
    
    void MainWindow::on_scanned_file_count(int fileCount)
    {
        m_progress_bar_->setMaximum(fileCount);
        m_progress_bar_->setValue(0);
        m_status_bar_->showMessage(tr("%1 Files Scanned...").arg(fileCount));
    }
    
    void MainWindow::on_group_found(Group group)
    {
        // Handle the group found event, e.g., update UI or store the group
        // This is a placeholder for actual logic to handle the found group.
        qDebug() << "Group found: " << group.name << " at path: " << group.path;
    }
    
    void MainWindow::set_current_folder(const QString& folder)
    {
        if (folder != m_current_folder_) {
            m_current_folder_ = folder;
            on_current_folder_changed();
        }
    }
}
    