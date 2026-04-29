#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "types/GroupTypes.h"

namespace photoboss {
	class Pipeline;

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

        static std::unique_ptr<Pipeline> create(const Config& config);

    };

}