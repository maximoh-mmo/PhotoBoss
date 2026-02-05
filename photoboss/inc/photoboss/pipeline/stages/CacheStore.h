#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
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
			QString id,
			QObject* parent = nullptr
		);
		~CacheStore() override = default;

	private:
		std::unique_ptr<IHashCache> m_cache;

		Queue<std::shared_ptr<HashedImageResult>>& m_input;
		Queue<std::shared_ptr<HashedImageResult>>& m_output;

		// Inherited via StageBase
		void run() override;
		void onStart() override;
		void onStop() override;
	};

}
