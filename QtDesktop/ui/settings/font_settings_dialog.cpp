#include "font_settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFont>

FontSettingsDialog::FontSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Font"));
    setMinimumSize(360, 280);
    resize(400, 300);
    setupUi();
}

QFont FontSettingsDialog::selectedFont() const
{
    QFont f = m_fontCombo->currentFont();
    f.setPointSize(m_sizeSpin->value());
    return f;
}

void FontSettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_fontCombo = new QFontComboBox(this);
    m_fontCombo->setCurrentFont(QFont("Microsoft YaHei"));
    formLayout->addRow(tr("Font family"), m_fontCombo);

    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(8, 48);
    m_sizeSpin->setValue(12);
    m_sizeSpin->setSuffix(" pt");
    formLayout->addRow(tr("Font size"), m_sizeSpin);

    m_colorCombo = new QComboBox(this);
    m_colorCombo->addItem(tr("White"), QColor(Qt::white).name());
    m_colorCombo->addItem(tr("Black"), QColor(Qt::black).name());
    m_colorCombo->addItem("#333333", "#333333");
    m_colorCombo->addItem("#666666", "#666666");
    m_colorCombo->addItem("#1a73e8", "#1a73e8");
    m_colorCombo->setCurrentIndex(1);
    formLayout->addRow(tr("Font color"), m_colorCombo);

    mainLayout->addLayout(formLayout);

    m_previewLabel = new QLabel(tr("Sample text preview"), this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(48);
    m_previewLabel->setStyleSheet(
        "QLabel {"
        "  border: 1px solid #e0e0e0;"
        "  border-radius: 4px;"
        "  background: white;"
        "  padding: 8px;"
        "}"
    );
    mainLayout->addWidget(m_previewLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_okBtn = new QPushButton(tr("OK"), this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);

    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    auto updatePreviewLambda = [this]() { updatePreview(); };
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, updatePreviewLambda);
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updatePreviewLambda);
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updatePreviewLambda);

    updatePreview();
}

void FontSettingsDialog::updatePreview()
{
    QFont f = selectedFont();
    m_previewLabel->setFont(f);

    QString colorName = m_colorCombo->currentData().toString();
    m_previewLabel->setStyleSheet(
        QString("QLabel {"
                "  border: 1px solid #e0e0e0;"
                "  border-radius: 4px;"
                "  background: white;"
                "  padding: 8px;"
                "  color: %1;"
                "}").arg(colorName)
    );
}
