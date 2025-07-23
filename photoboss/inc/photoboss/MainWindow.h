#pragma once

#include <QCheckBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QtCore>
#include <QtWidgets/QMainWindow>

#include "DiskReader.h"
#include "ImageScanner.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
namespace photoboss {
typedef struct {
    QString name;
    QString path;
} Group;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // MainWindow interface
    void WireConnections();
    void Init();
    void OnCurrentFolderChanged();
    void OnGroupFound(Group group);
    void SetCurrentFolder(const QString& folder);
    QString GetCurrentFolder() const { return m_current_folder_; }
    void OnScannedFileCount(int fileCount);
    void OnBrowse();
    void UpdateDiskReadProgress(int current, int total);
	void OnImageReady(const std::unique_ptr<DiskReadResult>& result);
	    
private:
    Ui::MainWindow *ui_ = nullptr;

    QString m_current_folder_;
    QPushButton* m_browse_button_ = nullptr;
    QPushButton* m_scan_button_ = nullptr;
    QFileDialog* m_file_dialog_ = nullptr;
    QProgressBar* m_progress_bar_ = nullptr;
    QCheckBox* m_include_subfolders_ = nullptr;
    ImageScanner* m_scanner_ = nullptr;
    QThread* m_scanner_thread_ = nullptr;
    QStatusBar* m_status_bar_ = nullptr;
    DiskReader* m_disk_reader_ = nullptr;
    QThread* m_reader_thread_ = nullptr;

    bool b_include_subfolders_ = false;
    bool b_scanning_ = false;
};
}