#include "settingsitem.h"
#include "settingsdialog.h"
#include <QCoreApplication>

// ============================================================================
// SettingsItemWidget
// ============================================================================

SettingsItemWidget::SettingsItemWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(52);
    setCursor(Qt::ArrowCursor);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(8);

    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    textLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("color: #1a1a1a; font-size: 13px; background: transparent;");

    m_descLabel = new QLabel(this);
    m_descLabel->setStyleSheet("color: #888888; font-size: 11px; background: transparent;");
    m_descLabel->setWordWrap(true);
    m_descLabel->hide();

    textLayout->addWidget(m_titleLabel);
    textLayout->addWidget(m_descLabel);

    layout->addLayout(textLayout, 1);
}

void SettingsItemWidget::setItemTitle(const QString &title)
{
    m_title = title;
    m_titleLabel->setText(title);
}

void SettingsItemWidget::setDescription(const QString &desc)
{
    m_description = desc;
    if (desc.isEmpty()) {
        m_descLabel->hide();
    } else {
        m_descLabel->setText(desc);
        m_descLabel->show();
        setFixedHeight(64);
    }
}

void SettingsItemWidget::setSeparatorVisible(bool visible)
{
    m_separatorVisible = visible;
    update();
}

void SettingsItemWidget::setChevronVisible(bool visible)
{
    m_chevronVisible = visible;
}

void SettingsItemWidget::setSettingsKey(const QString &key)
{
    m_settingsKey = key;
}

void SettingsItemWidget::configure(const QVariantMap &props)
{
    if (props.contains("key"))
        m_settingsKey = props["key"].toString();
}

void SettingsItemWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bgColor = m_hovered ? QColor(0, 0, 0, 8) : QColor(0, 0, 0, 0);
    painter.fillRect(rect(), bgColor);

    if (m_separatorVisible) {
        painter.setPen(QPen(QColor(0, 0, 0, 15), 1));
        int sepY = height() - 1;
        painter.drawLine(QPointF(16, sepY), QPointF(width() - 16, sepY));
    }

    if (m_chevronVisible) {
        painter.setPen(QPen(QColor(160, 160, 160), 2));
        int cx = width() - 28;
        int cy = height() / 2;
        painter.drawLine(cx, cy - 4, cx + 5, cy);
        painter.drawLine(cx + 5, cy, cx, cy + 4);
    }
}

void SettingsItemWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void SettingsItemWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

// ============================================================================
// SettingsCard
// ============================================================================

SettingsCard::SettingsCard(QWidget *parent)
    : QWidget(parent)
{
    m_itemLayout = new QVBoxLayout(this);
    m_itemLayout->setContentsMargins(0, 0, 0, 0);
    m_itemLayout->setSpacing(0);
}

void SettingsCard::addItem(SettingsItemWidget *item)
{
    m_itemLayout->addWidget(item);
}

void SettingsCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);

    painter.fillPath(path, QColor(252, 252, 252));
    painter.setPen(QPen(QColor(224, 224, 224), 1));
    painter.drawPath(path);
}

// ============================================================================
// SettingsGroup
// ============================================================================

SettingsGroup::SettingsGroup(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QLabel *groupTitle = new QLabel(title, this);
    groupTitle->setStyleSheet(
        "color: #666666;"
        "font-size: 12px;"
        "font-weight: bold;"
        "padding: 0 4px;"
        "background: transparent;"
    );
    layout->addWidget(groupTitle);

    m_card = new SettingsCard(this);
    layout->addWidget(m_card);
}

// ============================================================================
// SettingsItemFactory
// ============================================================================

SettingsItemFactory &SettingsItemFactory::instance()
{
    static SettingsItemFactory factory;
    return factory;
}

void SettingsItemFactory::registerType(const QString &typeName, Creator creator)
{
    m_creators[typeName] = std::move(creator);
}

SettingsItemWidget *SettingsItemFactory::create(const QString &typeName, QWidget *parent)
{
    auto it = m_creators.find(typeName);
    if (it != m_creators.end()) {
        return it.value()(parent);
    }
    return nullptr;
}

// ============================================================================
// SettingsContentRegistry
// ============================================================================

SettingsContentRegistry &SettingsContentRegistry::instance()
{
    static SettingsContentRegistry registry;
    return registry;
}

void SettingsContentRegistry::addEntry(const SettingsContentEntry &entry)
{
    m_entries.append(entry);
}

const QList<SettingsContentEntry> &SettingsContentRegistry::entries() const
{
    return m_entries;
}

// ============================================================================
// SettingsContentBuilder
// ============================================================================

static QVariant translateVariant(const QVariant &v, const char *context)
{
    if (v.typeId() == QMetaType::QString) {
        return QCoreApplication::translate(context,
            v.toString().toUtf8().constData());
    }
    if (v.typeId() == QMetaType::QStringList) {
        QStringList result;
        for (const QString &s : v.toStringList()) {
            result << QCoreApplication::translate(context,
                s.toUtf8().constData());
        }
        return result;
    }
    return v;
}

static QVariantMap translateProperties(const QVariantMap &props, const char *context)
{
    QVariantMap result;
    for (auto it = props.begin(); it != props.end(); ++it) {
        result[it.key()] = translateVariant(it.value(), context);
    }
    return result;
}

void SettingsContentBuilder::build(SettingsDialog *dialog)
{
    const char *ctx = "SettingsDialog";
    const auto &entries = SettingsContentRegistry::instance().entries();

    QHash<QString, SettingsGroup *> groups;

    for (const auto &entry : entries) {
        SettingsGroup *group = groups.value(entry.group);
        if (!group) {
            QString translatedGroup = QCoreApplication::translate(
                ctx, entry.group.toUtf8().constData());
            group = dialog->addGroup(translatedGroup);
            groups[entry.group] = group;
        }

        auto *item = SettingsItemFactory::instance().create(entry.itemType, dialog);
        if (!item)
            continue;

        item->setItemTitle(QCoreApplication::translate(ctx, entry.title.toUtf8().constData()));
        item->setDescription(QCoreApplication::translate(ctx, entry.description.toUtf8().constData()));
        item->configure(translateProperties(entry.properties, ctx));

        dialog->addItemToGroup(group, item);
    }
}
