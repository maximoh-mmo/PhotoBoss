#pragma once
#include <QDialog>
#include "ui_SettingsSelection.h"
#include "hashing/HashMethod.h"
#include "util/DataTypes.h"
#include <QCheckBox>


QT_BEGIN_NAMESPACE
namespace Ui { class SettingsSelection; }
QT_END_NAMESPACE
namespace photoboss
{
    class SettingsSelection;
}

namespace photoboss {

    class SettingsSelection : public QDialog
    {
        Q_OBJECT
    public:
        explicit SettingsSelection(QWidget* parent = nullptr);

		// Retrieve the set of enabled hash method keys
		std::set<QString> getEnabledHashMethods() const { return m_enabled_hash_keys_; }

        ~SettingsSelection() override;

    signals:
        void settingsApplied(const std::set<QString>& enabledKeys);

    private slots:
        void onApplyClicked();
		void onCheckboxToggled(bool checked);

    private:
        void updateEnabledHashMethods();

    private:
        Ui::SettingsSelection* ui_;

        // UI state only — no hashing logic
        std::map<QString, QCheckBox*> m_checkboxes_;
        std::set<QString> m_enabled_hash_keys_;
    };
}