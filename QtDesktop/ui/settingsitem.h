#ifndef SETTINGSITEM_H
#define SETTINGSITEM_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>
#include <QHash>
#include <QVariantMap>
#include <functional>

#include "../interfaces/isettingsitem.h"

// ============================================================================
// SettingsItemWidget - base widget implementing ISettingsItem
// ============================================================================

class SettingsItemWidget : public QWidget, public ISettingsItem
{
    Q_OBJECT

public:
    explicit SettingsItemWidget(QWidget *parent = nullptr);
    ~SettingsItemWidget() override = default;

    void setItemTitle(const QString &title);
    void setDescription(const QString &desc);
    QString itemTitle() const { return m_title; }

    void setSettingsKey(const QString &key);
    QString settingsKey() const override { return m_settingsKey; }

    void apply() override {}
    void reset() override {}
    bool isModified() const override { return m_modified; }

    void setSeparatorVisible(bool visible);
    void setChevronVisible(bool visible);
    virtual void configure(const QVariantMap &props) override;

signals:
    void valueChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    QString m_title;
    QString m_description;
    QString m_settingsKey;
    bool m_modified = false;
    bool m_hovered = false;
    bool m_separatorVisible = true;
    bool m_chevronVisible = false;

    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
};

// ============================================================================
// SettingsCard - Win11-style rounded container
// ============================================================================

class SettingsCard : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsCard(QWidget *parent = nullptr);

    void addItem(SettingsItemWidget *item);
    QVBoxLayout *itemLayout() const { return m_itemLayout; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVBoxLayout *m_itemLayout = nullptr;
};

// ============================================================================
// SettingsGroup - group title + card
// ============================================================================

class SettingsGroup : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsGroup(const QString &title, QWidget *parent = nullptr);

    SettingsCard *card() const { return m_card; }

private:
    SettingsCard *m_card = nullptr;
};

// ============================================================================
// SettingsItemFactory - factory with registration macro
// ============================================================================

class SettingsItemFactory
{
public:
    using Creator = std::function<SettingsItemWidget*(QWidget*)>;

    static SettingsItemFactory &instance();

    void registerType(const QString &typeName, Creator creator);

    template <typename T>
    void registerClass(const QString &typeName)
    {
        m_creators[typeName] = [](QWidget *parent) -> SettingsItemWidget* {
            return new T(parent);
        };
    }

    SettingsItemWidget *create(const QString &typeName, QWidget *parent = nullptr);

private:
    SettingsItemFactory() = default;
    QHash<QString, Creator> m_creators;
};

#define REGISTER_SETTINGS_ITEM(ClassName, TypeName) \
    static struct _Reg_##ClassName { \
        _Reg_##ClassName() { \
            SettingsItemFactory::instance().registerClass<ClassName>(TypeName); \
        } \
    } _reg_instance_##ClassName;

// ============================================================================
// SettingsContentRegistry - stores what items to show in the settings dialog
// ============================================================================

struct SettingsContentEntry
{
    QString group;
    QString itemType;
    QString title;
    QString description;
    QVariantMap properties;
};

class SettingsContentRegistry
{
public:
    static SettingsContentRegistry &instance();

    void addEntry(const SettingsContentEntry &entry);
    const QList<SettingsContentEntry> &entries() const;

private:
    SettingsContentRegistry() = default;
    QList<SettingsContentEntry> m_entries;
};

class SettingsDialog;
class SettingsContentBuilder
{
public:
    static void build(class SettingsDialog *dialog);
};

#define _TOKEN_CONCAT(a, b) _TOKEN_CONCAT_IMPL(a, b)
#define _TOKEN_CONCAT_IMPL(a, b) a##b

#define _REG_CONTENT(line, Group, Type, Title, Desc, ...) \
    static const int _TOKEN_CONCAT(_regContent_, line) = []() -> int { \
        SettingsContentRegistry::instance().addEntry({ \
            Group, Type, Title, Desc, \
            QVariantMap{__VA_ARGS__}}); \
        return 0; \
    }();

#define REGISTER_SETTINGS_CONTENT(Group, Type, Title, Desc, ...) \
    _REG_CONTENT(__LINE__, Group, Type, Title, Desc, ##__VA_ARGS__)

#endif // SETTINGSITEM_H
