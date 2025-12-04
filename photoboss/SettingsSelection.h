#pragma once
#include <QDialog>
#include "ui_SettingsSelection.h"
#include "HashMethod.h"
#include <QCheckBox>


QT_BEGIN_NAMESPACE
namespace Ui { class SettingsSelection; }
QT_END_NAMESPACE
namespace photoboss
{
    class SettingsSelection : public QDialog
    {
		Q_OBJECT

        public:
            SettingsSelection(const std::vector<std::shared_ptr<HashMethod>>& hashMethods, QWidget* parent = nullptr);
            ~SettingsSelection() override;
            void Init();
        private:
		Ui::SettingsSelection *ui_ = nullptr;
		QVBoxLayout* m_layout_ = nullptr;
		std::vector<QCheckBox*> m_checkboxes_;
		std::vector<std::shared_ptr<HashMethod>> m_methods_;
        signals:
        void open(const std::vector<std::shared_ptr<HashMethod>>& methods);
        void settingsApplied(const std::vector<std::shared_ptr<HashMethod>>& selectedMethods);

    };
}