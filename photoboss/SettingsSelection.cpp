#include "SettingsSelection.h"

photoboss::SettingsSelection::SettingsSelection(const std::vector<std::shared_ptr<HashMethod>>& hashMethods, QWidget* parent) : QDialog(parent)
, ui_(new Ui::SettingsSelection), m_methods_(hashMethods)
{
	ui_->setupUi(this);
	Init();
}

photoboss::SettingsSelection::~SettingsSelection()
{
}

void photoboss::SettingsSelection::Init()
{
	// get all implementaitons from hash methods, and create checkboxes for each
	m_layout_ = new QVBoxLayout(ui_->scrollAreaWidgetContents);
	for (const auto& method : m_methods_) {
		auto checkbox = new QCheckBox(method->getName(), this);
		checkbox->setChecked(method->isEnabled());
		m_layout_->addWidget(checkbox);
	}
	connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
		// gather selected methods
		std::vector<std::shared_ptr<HashMethod>> selectedMethods;
		int index = 0;
		for (const auto& method : m_methods_) {
			auto checkbox = qobject_cast<QCheckBox*>(m_layout_->itemAt(index)->widget());
			if (checkbox && checkbox->isChecked()) {
				method->setEnabled(true);
				selectedMethods.push_back(method);
			}
			else {
				method->setEnabled(false);
			}
			++index;
		}
		this->accept();
		});
}