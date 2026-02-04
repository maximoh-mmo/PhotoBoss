#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "pipeline/stages/Pipeline.h"
#include "caching/IHashCache.h"

namespace photoboss
{
	class CacheStore : public Transform<std::shared_ptr<HashedImageResult>, std::shared_ptr<HashedImageResult>>
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

		// Inherited via Transform
		std::shared_ptr<HashedImageResult> transform(const std::shared_ptr<HashedImageResult>& item) override;

		void onStart() override
		{
			qDebug() << "store registered";
			m_output.register_producer();
		}
		void onStop() override
		{
			qDebug() << "store deregistered";
			m_output.producer_done();
		}
	};

}
