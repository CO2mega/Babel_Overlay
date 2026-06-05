#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setMinimumSize(420, 320);
    resize(500, 420);
    setWindowTitle(tr("settings"));

    setupUi();
}

SettingsDialog::~SettingsDialog()
{
}

QScrollArea *SettingsDialog::contentArea() const
{
    return m_contentArea;
}

void SettingsDialog::onApply()
{
    // TODO: implement apply logic
}

void SettingsDialog::onOk()
{
    // TODO: implement ok logic
}

void SettingsDialog::onCancel()
{
    close();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    m_contentArea = new QScrollArea(this);
    m_contentArea->setWidgetResizable(true);
    m_contentArea->setFrameShape(QFrame::NoFrame);
    m_contentArea->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical {"
        "  width: 6px;"
        "  background: transparent;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #c0c0c0;"
        "  border-radius: 3px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    );

    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    m_contentArea->setWidget(scrollContent);

    mainLayout->addWidget(m_contentArea);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(8);

    QString btnStyle =
        "QPushButton {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-size: 13px;"
        "  color: #333333;"
        "  background: #f5f5f5;"
        "}"
        "QPushButton:hover {"
        "  background: #e8e8e8;"
        "  border-color: #a0a0a0;"
        "}";

    m_applyBtn = new QPushButton(tr("Apple"), this);
    m_okBtn = new QPushButton(tr("OK"), this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);

    m_applyBtn->setStyleSheet(btnStyle);
    m_okBtn->setStyleSheet(btnStyle);
    m_cancelBtn->setStyleSheet(btnStyle);

    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_okBtn, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);

    btnLayout->addStretch();
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}
