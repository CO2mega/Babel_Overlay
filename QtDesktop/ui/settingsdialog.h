#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QScrollArea>
#include <QPushButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QScrollArea *contentArea() const;

private slots:
    void onApply();
    void onOk();
    void onCancel();

private:
    void setupUi();

    QScrollArea *m_contentArea;
    QPushButton *m_applyBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
};

#endif // SETTINGSDIALOG_H
