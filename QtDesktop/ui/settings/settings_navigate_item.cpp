#include "settings_navigate_item.h"
#include <QMouseEvent>

REGISTER_SETTINGS_ITEM(SettingsNavigateItem, "navigate")

SettingsNavigateItem::SettingsNavigateItem(QWidget *parent)
    : SettingsItemWidget(parent)
{
    m_chevronVisible = true;
    setCursor(Qt::PointingHandCursor);
}

void SettingsNavigateItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    SettingsItemWidget::mousePressEvent(event);
}

void SettingsNavigateItem::setTarget(const QString &target)
{
    m_target = target;
}

void SettingsNavigateItem::configure(const QVariantMap &props)
{
    if (props.contains("target"))
        m_target = props["target"].toString();
}
