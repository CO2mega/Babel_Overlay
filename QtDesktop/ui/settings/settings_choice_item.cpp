#include "settings_choice_item.h"

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
    m_combo->addItems(choices);
}

QString SettingsChoiceItem::currentChoice() const
{
    return m_combo->currentText();
}

void SettingsChoiceItem::setCurrentChoice(const QString &choice)
{
    m_combo->setCurrentText(choice);
    m_defaultChoice = choice;
}

void SettingsChoiceItem::apply()
{
    m_defaultChoice = m_combo->currentText();
    m_modified = false;
}

void SettingsChoiceItem::reset()
{
    m_combo->setCurrentText(m_defaultChoice);
    m_modified = false;
}

bool SettingsChoiceItem::isModified() const
{
    return m_combo->currentText() != m_defaultChoice;
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
