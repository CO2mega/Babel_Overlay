#include "settings_input_item.h"

REGISTER_SETTINGS_ITEM(SettingsInputItem, "input")

SettingsInputItem::SettingsInputItem(QWidget *parent)
    : SettingsItemWidget(parent)
{
    m_input = new QLineEdit(this);
    m_input->setFixedWidth(200);
    m_input->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  background: #f5f5f5;"
        "  font-size: 12px;"
        "}"
    );

    QHBoxLayout *lay = qobject_cast<QHBoxLayout*>(layout());
    if (lay) {
        lay->addWidget(m_input);
    }

    connect(m_input, &QLineEdit::textChanged, this, &SettingsInputItem::valueChanged);
}

void SettingsInputItem::setText(const QString &text)
{
    m_input->setText(text);
    m_defaultText = text;
}

QString SettingsInputItem::text() const
{
    return m_input->text();
}

void SettingsInputItem::setPlaceholder(const QString &placeholder)
{
    m_input->setPlaceholderText(placeholder);
}

void SettingsInputItem::apply()
{
    m_defaultText = m_input->text();
    m_modified = false;
}

void SettingsInputItem::reset()
{
    m_input->setText(m_defaultText);
    m_modified = false;
}

bool SettingsInputItem::isModified() const
{
    return m_input->text() != m_defaultText;
}

void SettingsInputItem::configure(const QVariantMap &props)
{
    SettingsItemWidget::configure(props);
    if (props.contains("text"))
        setText(props["text"].toString());
    if (props.contains("placeholder"))
        setPlaceholder(props["placeholder"].toString());
}
