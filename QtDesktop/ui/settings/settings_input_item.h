#ifndef SETTINGS_INPUT_ITEM_H
#define SETTINGS_INPUT_ITEM_H

#include <QLineEdit>
#include "../settingsitem.h"

class SettingsInputItem : public SettingsItemWidget
{
    Q_OBJECT

public:
    explicit SettingsInputItem(QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const;
    void setPlaceholder(const QString &placeholder);

    void apply() override;
    void reset() override;
    bool isModified() const override;
    void configure(const QVariantMap &props) override;

private:
    QLineEdit *m_input = nullptr;
    QString m_defaultText;
};

#endif // SETTINGS_INPUT_ITEM_H
