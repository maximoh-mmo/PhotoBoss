#pragma once
#include <QWidget>
#include <QVector>
#include <qlayout.h>
#include <Qlabel>
#include <qpushbutton>
#include "util/GroupTypes.h"

namespace photoboss {

 	class ImageThumbWidget;

 	class GroupWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit GroupWidget(const ImageGroup& group, QWidget* parent = nullptr);

int countSelectedForDeletion() const;
 		QVector<ImageEntry> imagesMarkedForDeleteEntries() const;
		void updateGroup(const ImageGroup& group);
	
	signals:
		void previewImage(const ImageEntry& image);
		void selectionChanged();

	private slots:
		void onThumbClicked(ImageThumbWidget* clicked);
		void onThumbSelectionChanged();

	private:
		QVector<ImageThumbWidget*> m_thumbs;
		QLabel* m_header;
		QHBoxLayout* m_currentRow;
		QVBoxLayout* m_rootLayout;
	};
}