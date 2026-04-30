#pragma once
#include <QObject>
#include <QThread>
#include "types/DataTypes.h"

namespace photoboss {
	class Pipeline;
    class IUiUpdateSink;
	class StageBase;
 
    class PipelineFactory : public QObject {
		Q_OBJECT
    public:
        enum class StorageStrategy {
            Sequential,  // HDD - minimize seeks
            Parallel     // SSD - maximize throughput
        };

        struct Config {
            ScanRequest request;
            StorageStrategy storage;
        };

		explicit PipelineFactory(QObject* parent = nullptr);
        ~PipelineFactory();

        static std::unique_ptr<Pipeline> create(const Config& config, IUiUpdateSink* sink = nullptr);
        static void moveToThread(Pipeline* pipeline, StageBase* stage, QThread* thread = nullptr);
    };

}