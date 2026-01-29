#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "pipeline/stages/Pipeline.h"
#include "caching/IHashCache.h"
#include "hashing/HashRegistry.h"

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
			IHashCache& cache,
			const std::vector<HashRegistry::Entry>& activeMethods,
			QString id,
			QObject* parent = nullptr
		);

	public slots:
		void Run();

	private:
		Queue<FileIdentityBatchPtr>& m_inputQueue;
		Queue<FileIdentityBatchPtr>& m_diskReadQueue;
		Queue< std::shared_ptr<HashedImageResult>>& m_resultQueue;
		std::vector<HashRegistry::Entry> m_activeHashMethods;
		IHashCache& m_cache;
	};

}
