#pragma once
#include <QWidget>
#include <QVector>
#include <QMap>
#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include "types/GroupTypes.h"

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
		const QMap<QString, ImageThumbWidget*>& thumbsByPath() const { return m_thumbsByPath_; }
	
	signals:
		void previewImage(const ImageEntry& image);
		void selectionChanged();

	private slots:
		void onThumbClicked(ImageThumbWidget* clicked);
		void onThumbSelectionChanged();

	private:
		QVector<ImageThumbWidget*> m_thumbs_;
		QMap<QString, ImageThumbWidget*> m_thumbsByPath_;
		QLabel* m_header_;
		QHBoxLayout* m_currentRow_;
		QVBoxLayout* m_rootLayout_;
		bool m_userModified_ = false;
	};
}