#pragma once

#include <memory>

#include <QtCore>
#include <QtWidgets/QMainWindow>
#include <QProgressBar>
#include <QStatusBar>
#include <QLabel>

#include "types/DataTypes.h"
#include "types/GroupTypes.h"
#include "pipeline/Pipeline.h"
#include "ui/UiSnapshot.h"

class QPushButton;
class QSplitter;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace photoboss {

class PipelineController;
class ThumbnailManager;
class PreviewPane;
class DeletionService;
class ProgressCounterWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(std::unique_ptr<PipelineController> controller,
               std::unique_ptr<ThumbnailManager> thumbnailManager,
               std::unique_ptr<PreviewPane> previewPane,
               std::unique_ptr<DeletionService> deletionService,
               QWidget* parent = nullptr);
    ~MainWindow();

    QString GetCurrentFolder() const { return m_current_folder_; }

private slots:
    void applySnapshot(const UiSnapshot& snap);
    void onPipelineStateChanged(Pipeline::PipelineState state);
    void onGroupSelectionChanged();

private:
    void Init();
    void WireConnections();
    void OnBrowse();
    void SetCurrentFolder(const QString& folder);
    void OnCurrentFolderChanged();
    void clearResults();
    void updateDeleteButtonState();
    void updatePhaseProgress(const UiSnapshot& snap);

    Ui::MainWindow* m_ui_ = nullptr;
    std::unique_ptr<PipelineController> m_pipeline_controller_;
    std::unique_ptr<ThumbnailManager> m_thumbnailManager_;
    std::unique_ptr<PreviewPane> m_preview_pane_;
    std::unique_ptr<DeletionService> m_deletionService_;

    QString m_current_folder_;

    QPushButton* m_browse_button_ = nullptr;
    QPushButton* m_scan_button_ = nullptr;
    QPushButton* m_btn_delete_ = nullptr;
    QProgressBar* m_progress_bar_ = nullptr;
    QStatusBar* m_status_bar_ = nullptr;
    QLabel* m_delete_count_label_ = nullptr;

    QMap<Pipeline::Phase, ProgressCounterWidget*> m_phase_indicators_;

    QSplitter* m_splitter_ = nullptr;

    UiSnapshot m_lastSnapshot_;
    Pipeline::PipelineState m_lastPipelineState_ = Pipeline::PipelineState::Stopped;
    int m_lastSelectionCount_ = -1;
};

} // namespace photoboss
