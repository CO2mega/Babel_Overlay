#ifndef SETTINGS_CHOICE_ITEM_H
#define SETTINGS_CHOICE_ITEM_H

#include <QComboBox>
#include "../settingsitem.h"

class SettingsChoiceItem : public SettingsItemWidget
{
    Q_OBJECT

public:
    explicit SettingsChoiceItem(QWidget *parent = nullptr);

    void setChoices(const QStringList &choices);
    QString currentChoice() const;
    void setCurrentChoice(const QString &choice);

    void apply() override;
    void reset() override;
    bool isModified() const override;
    void configure(const QVariantMap &props) override;

private:
    QComboBox *m_combo = nullptr;
    QString m_defaultChoice;
};

#endif // SETTINGS_CHOICE_ITEM_H
