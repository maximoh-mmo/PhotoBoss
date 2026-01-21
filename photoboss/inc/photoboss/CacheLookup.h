#pragma once
#include <QObject>
#include "Queue.h"
#include "DataTypes.h"

namespace photoboss
{
	class CacheLookup : public QObject
	{
		Q_OBJECT
	public:
		CacheLookup(
			Queue<FingerprintBatchPtr>& inputQueue,
			Queue<FingerprintBatchPtr>& diskReadQueue,
			QObject* parent = nullptr
		);

	public slots:
		void Run();

	signals:
		void cachedImage(std::shared_ptr<HashedImageResult> result);
		void status(QString message);
	private:
		Queue<FingerprintBatchPtr>& m_inputQueue;
		Queue<FingerprintBatchPtr>& m_diskReadQueue;
	};

}
