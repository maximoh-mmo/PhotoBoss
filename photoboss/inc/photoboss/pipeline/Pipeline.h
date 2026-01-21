#pragma once
#include <vector>
#include <QThread>
#include "Queue.h"
#include "HashWorker.h"
#include "DataTypes.h"
#include "HashMethod.h"

namespace photoboss {
    class DirectoryScanner;
    class DiskReader;
    class ResultProcessor;
	class CacheLookup;

    struct Pipeline {
        // Queues
        Queue<FingerprintBatchPtr> scanQueue;
        Queue<FingerprintBatchPtr> diskQueue;
        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;

        // Threads
        QThread scannerThread;
        QThread readerThread;
		QThread resultThread;
		QThread cacheThread;

        // Workers
        DirectoryScanner* scanner = nullptr;
        DiskReader* reader = nullptr;
		ResultProcessor* resultProcessor = nullptr;
		CacheLookup* cacheLookup = nullptr;
        std::vector<HashWorker*> hashWorkers;

		Pipeline() = default;
        Pipeline(size_t scanCap,
            size_t diskCap,
            size_t readCap,
            size_t resultCap)
            : scanQueue(scanCap)
            , diskQueue(diskCap)
            , readQueue(readCap)
            , resultQueue(resultCap)
        
        { 
        }
    };
}