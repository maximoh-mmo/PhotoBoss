#pragma once
#include <QObject>
#include "Queue.h"

#include "DataTypes.h"
namespace photoboss {
    class ResultProcessor : public QObject
    {
        Q_OBJECT
    public:
        explicit ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue, QObject* parent = nullptr);
        void stop();
    public slots:
        void Run();
        void HandleResult(const std::shared_ptr<HashedImageResult>& result);

    signals:
        void hash_stored(QString path);
        void duplicate_found(QString a, QString b);

    private:
		Queue<std::shared_ptr<HashedImageResult>>& queue_;
    };
}
