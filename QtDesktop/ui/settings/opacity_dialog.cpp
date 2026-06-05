#include "opacity_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>

OpacityDialog::OpacityDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Window opacity"));
    setMinimumSize(300, 200);
    resize(360, 220);
    setupUi();
}

int OpacityDialog::opacity() const
{
    return m_slider->value();
}

void OpacityDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sliderLayout->setSpacing(10);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(10, 100);
    m_slider->setValue(100);
    m_slider->setTickPosition(QSlider::TicksBelow);
    m_slider->setTickInterval(10);

    m_valueLabel = new QLabel(tr("100%"), this);
    m_valueLabel->setFixedWidth(40);
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sliderLayout->addWidget(m_slider);
    sliderLayout->addWidget(m_valueLabel);

    mainLayout->addLayout(sliderLayout);

    m_previewLabel = new QLabel("Preview", this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(48);
    m_previewLabel->setStyleSheet(
        "QLabel {"
        "  border: 1px solid #e0e0e0;"
        "  border-radius: 4px;"
        "  background: #1976d2;"
        "  color: white;"
        "  padding: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
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

    connect(m_slider, &QSlider::valueChanged, this, &OpacityDialog::updatePreview);
    updatePreview();
}

void OpacityDialog::updatePreview()
{
    int val = m_slider->value();
    m_valueLabel->setText(QString("%1%").arg(val));

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(val / 100.0);
    m_previewLabel->setGraphicsEffect(effect);
}
