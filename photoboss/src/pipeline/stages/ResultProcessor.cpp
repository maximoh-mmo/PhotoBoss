#include "pipeline/stages/ResultProcessor.h"
#include "hashing/HashCatalog.h"
#include "pipeline/SimilarityEngine.h"

namespace photoboss {
    ResultProcessor::ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
        QString id,
        QObject* parent) :
        StageBase(std::move(id), parent),
        m_input(queue),
        m_items()
    {
    }

    QString humanSize(qint64 bytes)
    {
        constexpr double KB = 1024.0;
        constexpr double MB = KB * 1024.0;
        constexpr double GB = MB * 1024.0;

        if (bytes >= GB) return QString::number(bytes / GB, 'f', 2) + " GB";
        if (bytes >= MB) return QString::number(bytes / MB, 'f', 2) + " MB";
        if (bytes >= KB) return QString::number(bytes / KB, 'f', 1) + " KB";
        return QString::number(bytes) + " B";
    }


    void ResultProcessor::run() {
        std::shared_ptr<HashedImageResult> item;
        while (m_input.wait_and_pop(item)) {
            Q_ASSERT(!item->hashes.empty());
            m_items.push_back(std::move(item));
        }
        SimilarityEngine engine;
        auto groups = engine.group(m_items);

        // ---- DEBUG OUTPUT ----
        qDebug().noquote() << "\n========== SIMILARITY GROUPS ==========";

        int groupIndex = 1;
        for (const auto& group : groups) {
            if (group.images.size() <= 1)
                continue;
            qDebug().noquote()
                << "\n--- Group"
                << groupIndex++
                << "| Images:"
                << group.images.size()
                << "---";

            if (!group.images.empty()) {
                const auto& rep = group.images[group.bestIndex];
                qDebug().noquote()
                    << " Representative:"
                    << rep.path
                    << "|"
                    << rep.resolution.width() << "x" << rep.resolution.height()
                    << "|"
                    << humanSize(rep.fileSize);
            }

            for (size_t i = 0; i < group.images.size(); ++i) {
                const auto& img = group.images[i];

                qDebug().noquote()
                    << " "
                    << (img.isBest ? "*" : " ")
                    << QString("[%1]").arg(i)
                    << img.path
                    << "|"
                    << img.resolution.width() << "x" << img.resolution.height()
                    << "|"
                    << humanSize(img.fileSize);
            }
        }

        qDebug().noquote() << "======================================\n";
        int duplicateGroups = 0;
        int duplicateImages = 0;

        for (const auto& g : groups) {
            if (g.images.size() > 1) {
                ++duplicateGroups;
                duplicateImages += static_cast<int>(g.images.size());
            }
        }

        qDebug().noquote()
            << "\nFound"
            << duplicateGroups
            << "duplicate groups covering"
            << duplicateImages
            << "images.";
        emit groupingFinished(std::move(groups));
    }

}