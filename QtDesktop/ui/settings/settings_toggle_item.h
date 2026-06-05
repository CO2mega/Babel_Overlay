#ifndef SETTINGS_TOGGLE_ITEM_H
#define SETTINGS_TOGGLE_ITEM_H

#include <QCheckBox>
#include "../settingsitem.h"

class SettingsToggleItem : public SettingsItemWidget
{
    Q_OBJECT

public:
    explicit SettingsToggleItem(QWidget *parent = nullptr);

    void setChecked(bool checked);
    bool isChecked() const;

    void apply() override;
    void reset() override;
    bool isModified() const override;
    void configure(const QVariantMap &props) override;

private:
    QCheckBox *m_toggle = nullptr;
    bool m_defaultValue = false;
};

#endif // SETTINGS_TOGGLE_ITEM_H
