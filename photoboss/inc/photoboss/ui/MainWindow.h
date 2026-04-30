#pragma once

#include <QCheckBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QtCore>
#include <QtWidgets/QMainWindow>
#include "types/DataTypes.h"
#include "types/GroupTypes.h"
#include <QSplitter>
#include <qscrollarea.h>
#include <qlayout.h>
#include <QPixmap>
#include <QMap>
#include <QMultiMap>
#include <deque>
#include "ui/PreviewPane.h"
#include "ui/ProgressCounterWidget.h"
#include "pipeline/Pipeline.h"
#include "ui/UiUpdateQueue.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
namespace photoboss {
	class PipelineFactory;
    class GroupWidget;
    class ImageThumbWidget;
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
        void onGroupAdded(const ImageGroup& group);
        void onGroupUpdated(const ImageGroup& group);
        void onThumbnailReady(const ThumbnailResult& result);
        void clearResults();
        QString GetCurrentFolder() const { return m_current_folder_; }
        void OnBrowse();
        void UpdateProgressBar(int current, int total);

    private slots:
        void processBatch();
        void applySnapshot(const UiUpdateQueue::Snapshot& snap);
        void onPipelineStateChanged(Pipeline::PipelineState state);
        void onDeleteClicked();
        void onGroupSelectionChanged();

    private:
        void updateDeleteButtonState();
        int countSelectedForDeletion() const;
        QVector<ImageEntry> collectSelectedForDeletion() const;
        void resetPhaseIndicators();


        Ui::MainWindow* m_ui_ = nullptr;
        std::unique_ptr<PipelineController> m_pipeline_controller_ = nullptr;
		std::unique_ptr<PipelineFactory> m_pipeline_factory_ = nullptr;

        QString m_current_folder_;
        bool m_scan_found_duplicates_ = false;

        QPushButton* m_browse_button_ = nullptr;
        QPushButton* m_scan_button_ = nullptr;
        QPushButton* m_factory_scan_button_ = nullptr;
        QPushButton* m_btn_delete_ = nullptr;
        QProgressBar* m_progress_bar_ = nullptr;
        QStatusBar* m_status_bar_ = nullptr;
        QLabel* m_delete_count_label_ = nullptr;

        // Phase indicators
		QMap<Pipeline::Phase, ProgressCounterWidget*> m_phase_indicators_;

        QScrollArea* m_body_ = nullptr;
        QSplitter* m_splitter_ = nullptr;

        // Left
        QScrollArea* m_thumbnail_scroll_ = nullptr;
        QWidget* m_thumbnail_container_ = nullptr;
        QVBoxLayout* m_thumbnail_layout_ = nullptr;

        // Right
        QMap<quint64, GroupWidget*> m_groupWidgets_;
        QMultiMap<QString, ImageThumbWidget*> m_thumbnailWaiters_;
        QMap<QString, QPixmap> m_thumbnailCache_;
        PreviewPane* m_preview_pane_ = nullptr;

        std::deque<ImageGroup> m_pendingGroups_;
        QTimer* m_batchTimer_ = nullptr;
        // Cache of the previous UI snapshot for early‑out
        UiUpdateQueue::Snapshot m_lastSnapshot_;
        // Remember previous selection count to avoid redundant delete‑button updates
        int m_lastSelectionCount_ = -1;
        QTimer* m_uiPollTimer_ = nullptr;

    };
}