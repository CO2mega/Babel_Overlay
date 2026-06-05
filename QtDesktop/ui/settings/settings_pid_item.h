#ifndef SETTINGS_PID_ITEM_H
#define SETTINGS_PID_ITEM_H

#include <QLineEdit>
#include <QPushButton>
#include <windows.h>
#include "../settingsitem.h"

class SettingsPidItem : public SettingsItemWidget
{
    Q_OBJECT

public:
    explicit SettingsPidItem(QWidget *parent = nullptr);
    ~SettingsPidItem() override;

    QString pidText() const;
    void setPidText(const QString &text);

    void apply() override;
    void reset() override;
    bool isModified() const override;
    void configure(const QVariantMap &props) override;

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void enterPickMode();
    void leavePickMode();
    void capturePidAtCursor();

    QLineEdit *m_input = nullptr;
    QPushButton *m_pickBtn = nullptr;
    QString m_defaultPid;
    bool m_picking = false;
};

#endif // SETTINGS_PID_ITEM_H
