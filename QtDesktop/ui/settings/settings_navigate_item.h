#ifndef SETTINGS_NAVIGATE_ITEM_H
#define SETTINGS_NAVIGATE_ITEM_H

#include "../settingsitem.h"

class SettingsNavigateItem : public SettingsItemWidget
{
    Q_OBJECT

public:
    explicit SettingsNavigateItem(QWidget *parent = nullptr);

    QString target() const { return m_target; }
    void setTarget(const QString &target);
    void configure(const QVariantMap &props) override;

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_target;
};

#endif // SETTINGS_NAVIGATE_ITEM_H
