#pragma once
#include <QObject>
#include "util/Queue.h"
#include "types/DataTypes.h"
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
			Queue<FileIdentity>& input,
			Queue<FileIdentity>& diskOut,
			Queue< std::shared_ptr<HashedImageResult>>& resultOut,
			quint64 scanId,
			QObject* parent = nullptr
		);

	public slots:
		void doRun() override;

	private:
		Queue<FileIdentity>& m_inputQueue_;
		Queue<FileIdentity>& m_diskReadQueue_;
		Queue< std::shared_ptr<HashedImageResult>>& m_resultQueue_;
		std::unique_ptr<IHashCache> m_cache_;
		QList<QString> m_methods_;

		// Inherited via StageBase
		void onStop() override;
	};

}
