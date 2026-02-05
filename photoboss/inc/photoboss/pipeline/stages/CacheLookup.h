#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "caching/IHashCache.h"
#include "hashing/HashCatalog.h"
#include "pipeline/StageBase.h"

namespace photoboss
{
	class CacheLookup : public StageBase
	{
		Q_OBJECT
	public:
		CacheLookup(
			Queue<FileIdentityBatchPtr>& input,
			Queue<FileIdentityBatchPtr>& diskOut,
			Queue< std::shared_ptr<HashedImageResult>>& resultOut,
			QString id,
			QObject* parent = nullptr
		);

	public slots:
		void run();

	private:
		Queue<FileIdentityBatchPtr>& m_inputQueue;
		Queue<FileIdentityBatchPtr>& m_diskReadQueue;
		Queue< std::shared_ptr<HashedImageResult>>& m_resultQueue;
		std::unique_ptr<IHashCache> m_cache;
		QList<QString> m_methods;

		// Inherited via StageBase
		void onStart() override;
		void onStop() override;
	};

}
