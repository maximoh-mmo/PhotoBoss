#include "pipelinecontroller.h"
#include "directoryscanner.h"
#include "imageloader.h"
#include "imagecomparator.h"
namespace photoboss
{


    PipelineController::PipelineController(QObject* parent) : QObject(parent)
    {
        // create workers (no parent yet — parent them AFTER moving to thread)
        m_scanner_ = new DirectoryScanner();
        m_loader_ = new ImageLoader();
        m_comparator_ = new ImageComparator();

        setupThreads();
        setupConnections();
    }

    PipelineController::~PipelineController()
    {
        stop();

        m_scanner_thread_.quit();
        m_loader_thread_.quit();
        m_compare_thread_.quit();

        m_scanner_thread_.wait();
        m_loader_thread_.wait();
        m_compare_thread_.wait();
    }

    void PipelineController::setupThreads()
    {
        m_scanner_->moveToThread(&m_scanner_thread_);
        m_loader_->moveToThread(&m_loader_thread_);
        m_comparator_->moveToThread(&m_compare_thread_);

        m_scanner_thread_.start();
        m_loader_thread_.start();
        m_compare_thread_.start();
    }

    void PipelineController::setupConnections()
    {
        // ── PIPELINE FLOW ───────────────────────────────
        connect(this, &PipelineController::start,
            m_scanner_, &DirectoryScanner::scan);

        connect(m_scanner_, &DirectoryScanner::fileFound,
            m_loader_, &ImageLoader::loadImage);

        connect(m_loader_, &ImageLoader::imageReady,
            m_comparator_, &ImageComparator::compare);

        connect(m_comparator_, &ImageComparator::groupReady,
            this, &PipelineController::fileGroupReady);

        // ── STATUS MESSAGES TO UI ───────────────────────
        connect(m_scanner_, &DirectoryScanner::status,
            this, &PipelineController::progressUpdate);
        connect(m_loader_, &ImageLoader::status,
            this, &PipelineController::progressUpdate);
        connect(m_comparator_, &ImageComparator::status,
            this, &PipelineController::progressUpdate);

        // ── FINISH ──────────────────────────────────────
        connect(m_comparator_, &ImageComparator::finished,
            this, &PipelineController::finished);
    }

    void PipelineController::start(const QString& root_directory)
    {
        m_stop_requested_ = false;
        emit progressUpdate("Starting scan...");
        emit start(root_directory);
    }

    void PipelineController::stop()
    {
        m_stop_requested_ = true;
        m_scanner_->requestStop();
        m_loader_->requestStop();
        m_comparator_->requestStop();
    }
}