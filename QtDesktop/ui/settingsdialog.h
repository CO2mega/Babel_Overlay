#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QScrollArea>
#include <QPushButton>
#include <QList>
#include "settingsitem.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QScrollArea *contentArea() const;

    SettingsGroup *addGroup(const QString &title);
    void addItemToGroup(SettingsGroup *group, SettingsItemWidget *item);

private slots:
    void onApply();
    void onOk();
    void onCancel();

private:
    void setupUi();
    void buildContent();

    QScrollArea *m_contentArea;
    QWidget *m_scrollContent;
    QVBoxLayout *m_contentLayout;
    QPushButton *m_applyBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    QList<SettingsItemWidget *> m_allItems;
};

#endif // SETTINGSDIALOG_H
