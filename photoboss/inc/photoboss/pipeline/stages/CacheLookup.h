#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "PipelineStage.h"
#include "IHashCache.h"
#include "HashMethod.h"

namespace photoboss
{
	class CacheLookup : public PipelineStage
	{
		Q_OBJECT
	public:
		CacheLookup(
			Queue<FingerprintBatchPtr>& input,
			Queue<FingerprintBatchPtr>& diskOut,
			Queue< std::shared_ptr<HashedImageResult>>& resultOut,
			IHashCache& cache,
			const std::vector<HashRegistry::Entry>& activeMethods,
			QObject* parent = nullptr
		);

	public slots:
		void Run();

	private:
		Queue<FingerprintBatchPtr>& m_inputQueue;
		Queue<FingerprintBatchPtr>& m_diskReadQueue;
		Queue< std::shared_ptr<HashedImageResult>>& m_resultQueue;
		std::vector<HashRegistry::Entry> m_activeHashMethods;
		IHashCache& m_cache;
	};

}
