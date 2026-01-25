#include "ui/SettingsSelection.h"
#include "ui_SettingsSelection.h"
#include "hashing/HashRegistry.h"
#include <QVBoxLayout>
#include <QPushButton>

namespace photoboss {

	SettingsSelection::SettingsSelection(
		QWidget* parent)
		: QDialog(parent)
		, ui_(new Ui::SettingsSelection)
	{
		ui_->setupUi(this);

		const auto registered = HashRegistry::registeredNames();
		for (const auto& key : registered) {
			m_enabled_hash_keys_.insert(key);
		}

		updateEnabledHashMethods();

		connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &SettingsSelection::onApplyClicked);
	}

	SettingsSelection::~SettingsSelection()
	{
		delete ui_;
	}

	void SettingsSelection::onCheckboxToggled(bool checked)
	{
		QCheckBox* sender_cb = qobject_cast<QCheckBox*>(sender());
		if (!sender_cb) return;
		const QString key = sender_cb->text();
		if (checked) {
			qDebug() << "Enabled hash method:" << key;
			m_enabled_hash_keys_.insert(key);
		}
		else {
			qDebug() << "Disabled hash method:" << key;
			m_enabled_hash_keys_.erase(key);
		}
	}

	void SettingsSelection::updateEnabledHashMethods()
	{
		QLayout* layout = ui_->scrollAreaWidgetContents->layout();
		if (!layout) layout = new QVBoxLayout(ui_->scrollAreaWidgetContents);
		
		const auto registered = HashRegistry::registeredNames();
		QSet<QString> registryKeys(registered.begin(), registered.end());

		// 1 Remove checkboxes for hashes that no longer exist
		for (auto it = m_checkboxes_.begin(); it != m_checkboxes_.end(); ) {
			if (!registryKeys.contains(it->first)) {
				delete it->second;
				it = m_checkboxes_.erase(it);
			}
			else {
				++it;
			}
		}

		// 2 Add new checkboxes for newly registered hashes
		for (const QString& key : registered) {
			if (!m_checkboxes_.count(key)) {
				QCheckBox* cb = new QCheckBox(key, this);
				cb->setChecked(m_enabled_hash_keys_.count(key) > 0);
				connect(cb, &QCheckBox::toggled,
					this, &SettingsSelection::onCheckboxToggled);
				layout->addWidget(cb);
				m_checkboxes_[key] = cb;
			}
		}

		// 3 Sync checked state (in case config changed externally)
		for (auto& [key, cb] : m_checkboxes_) {
			cb->blockSignals(true);
			cb->setChecked(m_enabled_hash_keys_.count(key) > 0);
			cb->blockSignals(false);
		}
	}

	void SettingsSelection::onApplyClicked()
	{
		emit settingsApplied(m_enabled_hash_keys_);
		accept();
	}
}