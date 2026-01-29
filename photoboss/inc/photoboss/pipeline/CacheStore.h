#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "pipeline/stages/Pipeline.h"
#include "caching/IHashCache.h"
#include "hashing/HashRegistry.h"

namespace photoboss
{
	class CacheStore : public Transform<std::shared_ptr<HashedImageResult>, std::shared_ptr<HashedImageResult>>
	{
		Q_OBJECT
	public:
		CacheStore(
			Queue<std::shared_ptr<HashedImageResult>>& input,
			Queue<std::shared_ptr<HashedImageResult>>& output,
			IHashCache& cache,
			const std::vector<HashRegistry::Entry>& activeMethods,
			QString id,
			QObject* parent = nullptr
		);
		~CacheStore() override = default;

	private:
		std::vector<HashRegistry::Entry> m_activeHashMethods;
		IHashCache& m_cache;

		// Inherited via Transform
		std::shared_ptr<HashedImageResult> transform(const std::shared_ptr<HashedImageResult>& item) override;
	};

}
