#ifndef FONT_SETTINGS_DIALOG_H
#define FONT_SETTINGS_DIALOG_H

#include <QDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class FontSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FontSettingsDialog(QWidget *parent = nullptr);

    QFont selectedFont() const;

private:
    void setupUi();
    void updatePreview();

    QFontComboBox *m_fontCombo;
    QSpinBox *m_sizeSpin;
    QComboBox *m_colorCombo;
    QLabel *m_previewLabel;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
};

#endif // FONT_SETTINGS_DIALOG_H
