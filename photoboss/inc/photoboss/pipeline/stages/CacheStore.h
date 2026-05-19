#pragma once
#include <QObject>
#include <vector>
#include <utility>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "pipeline/StageBase.h"
#include "caching/IHashCache.h"

namespace photoboss
{
	class CacheStore : public StageBase
	{
		Q_OBJECT
	public:
		CacheStore(
			Queue<std::shared_ptr<HashedImageResult>>& input,
			Queue<std::shared_ptr<HashedImageResult>>& output,
			quint64 scanId,
			QObject* parent = nullptr
		);
		~CacheStore() override = default;

	private:
		void flushBatch();

		std::unique_ptr<IHashCache> m_cache_;

		Queue<std::shared_ptr<HashedImageResult>>& m_input_;
		Queue<std::shared_ptr<HashedImageResult>>& m_output_;
		std::vector<std::pair<HashedImageResult, QMap<QString, int>>> m_batch_;

		// Inherited via StageBase
		void doRun() override;
		void onStop() override;
	};
}
