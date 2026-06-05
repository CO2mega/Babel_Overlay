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
