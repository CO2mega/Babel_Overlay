#include "settings_toggle_item.h"

REGISTER_SETTINGS_ITEM(SettingsToggleItem, "toggle")

SettingsToggleItem::SettingsToggleItem(QWidget *parent)
    : SettingsItemWidget(parent)
{
    m_toggle = new QCheckBox(this);
    m_toggle->setStyleSheet(
        "QCheckBox::indicator { width: 38px; height: 20px; }"
    );
    QHBoxLayout *lay = qobject_cast<QHBoxLayout*>(layout());
    if (lay) {
        lay->addWidget(m_toggle);
    }
}

void SettingsToggleItem::setChecked(bool checked)
{
    m_toggle->setChecked(checked);
    m_defaultValue = checked;
}

bool SettingsToggleItem::isChecked() const
{
    return m_toggle->isChecked();
}

void SettingsToggleItem::apply()
{
    m_defaultValue = m_toggle->isChecked();
    m_modified = false;
}

void SettingsToggleItem::reset()
{
    m_toggle->setChecked(m_defaultValue);
    m_modified = false;
}

bool SettingsToggleItem::isModified() const
{
    return m_toggle->isChecked() != m_defaultValue;
}

void SettingsToggleItem::configure(const QVariantMap &props)
{
    if (props.contains("checked"))
        setChecked(props["checked"].toBool());
}
