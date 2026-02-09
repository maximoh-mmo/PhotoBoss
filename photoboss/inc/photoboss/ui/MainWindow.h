#pragma once

#include <QCheckBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QtCore>
#include <QtWidgets/QMainWindow>
#include "util/DataTypes.h"
#include "util/GroupTypes.h"
#include <QSplitter>
#include <qscrollarea.h>
#include <qlayout.h>
#include "ui/PreviewPane.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
namespace photoboss {
	class PipelineController;

    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow();

        // MainWindow interface
        void WireConnections();
        void Init();
        void OnCurrentFolderChanged();
        void SetCurrentFolder(const QString& folder);
        void onGroupingFinished(const std::vector<ImageGroup> groups);
        void clearResults();
        QString GetCurrentFolder() const { return m_current_folder_; }
        void OnScannedFileCount(int fileCount);
        void OnBrowse();
        void UpdateDiskReadProgress(int current, int total);

    private:
        Ui::MainWindow* ui_ = nullptr;
        std::unique_ptr<PipelineController> m_pipeline_controller_ = nullptr;

        QString m_current_folder_;

        QPushButton* m_browse_button_ = nullptr;
        QPushButton* m_scan_button_ = nullptr;
        QProgressBar* m_progress_bar_ = nullptr;
        QStatusBar* m_status_bar_ = nullptr;

        QScrollArea* m_body_ = nullptr;
        QSplitter* m_splitter_ = nullptr;

        // Left
        QScrollArea* m_thumbnail_scroll_ = nullptr;
        QWidget* m_thumbnail_container_ = nullptr;
        QVBoxLayout* m_thumbnail_layout_ = nullptr;

        // Right
        PreviewPane* m_preview_pane = nullptr;

    };
}