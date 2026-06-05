#include "settings_choice_item.h"
#include <QCoreApplication>

REGISTER_SETTINGS_ITEM(SettingsChoiceItem, "choice")

SettingsChoiceItem::SettingsChoiceItem(QWidget *parent)
    : SettingsItemWidget(parent)
{
    m_combo = new QComboBox(this);
    m_combo->setFixedWidth(160);
    m_combo->setStyleSheet(
        "QComboBox {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  background: #f5f5f5;"
        "  font-size: 12px;"
        "}"
    );

    QHBoxLayout *lay = qobject_cast<QHBoxLayout*>(layout());
    if (lay) {
        lay->addWidget(m_combo);
    }

    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsChoiceItem::valueChanged);
}

void SettingsChoiceItem::setChoices(const QStringList &choices)
{
    m_combo->clear();
    for (const QString &raw : choices) {
        QString display = QCoreApplication::translate("SettingsDialog",
            raw.toUtf8().constData());
        m_combo->addItem(display, raw);
    }
}

QString SettingsChoiceItem::currentChoice() const
{
    return m_combo->currentData().toString();
}

void SettingsChoiceItem::setCurrentChoice(const QString &choice)
{
    int idx = m_combo->findData(choice);
    if (idx >= 0) {
        m_combo->setCurrentIndex(idx);
    } else {
        m_combo->setCurrentText(choice);
    }
    m_defaultChoice = currentChoice();
}

void SettingsChoiceItem::apply()
{
    m_defaultChoice = currentChoice();
    m_modified = false;
}

void SettingsChoiceItem::reset()
{
    int idx = m_combo->findData(m_defaultChoice);
    if (idx >= 0)
        m_combo->setCurrentIndex(idx);
    m_modified = false;
}

bool SettingsChoiceItem::isModified() const
{
    return currentChoice() != m_defaultChoice;
}

void SettingsChoiceItem::configure(const QVariantMap &props)
{
    SettingsItemWidget::configure(props);
    if (props.contains("choices")) {
        setChoices(props["choices"].toStringList());
    }
    if (props.contains("current")) {
        setCurrentChoice(props["current"].toString());
    }
}
