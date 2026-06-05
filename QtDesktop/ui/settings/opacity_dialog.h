#ifndef OPACITY_DIALOG_H
#define OPACITY_DIALOG_H

#include <QDialog>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class OpacityDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpacityDialog(QWidget *parent = nullptr);

    int opacity() const;

private:
    void setupUi();
    void updatePreview();

    QSlider *m_slider;
    QLabel *m_valueLabel;
    QLabel *m_previewLabel;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
};

#endif // OPACITY_DIALOG_H
