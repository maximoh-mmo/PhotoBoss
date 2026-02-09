#pragma once
#include <QWidget>
#include <QVector>
#include <qlayout.h>
#include <Qlabel>
#include <qpushbutton>
#include "util/GroupTypes.h"

namespace photoboss {

	class ImageThumbWidget;
	struct HashedImageResult;

	class GroupWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit GroupWidget(const ImageGroup& group, QWidget* parent = nullptr);

		QVector<const HashedImageResult*> imagesMarkedForDelete() const;
	
	private slots:
		void onThumbClicked(ImageThumbWidget* clicked);

	private:
		QVector<ImageThumbWidget*> m_thumbs;
	};
}