#ifndef SETTINGS_NAVIGATE_ITEM_H
#define SETTINGS_NAVIGATE_ITEM_H

#include "../settingsitem.h"

class SettingsNavigateItem : public SettingsItemWidget
{
    Q_OBJECT

public:
    explicit SettingsNavigateItem(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // SETTINGS_NAVIGATE_ITEM_H
