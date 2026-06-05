#include "settingsdialog.h"
#include "settings/settings_navigate_item.h"
#include "settings/settings_toggle_item.h"
#include "settings/settings_choice_item.h"
#include "settings/settings_input_item.h"
#include "settings/font_settings_dialog.h"
#include "settings/opacity_dialog.h"
#include "config/config_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setMinimumSize(420, 320);
    resize(520, 460);
    setWindowTitle(tr("settings"));

    setupUi();
    buildContent();
}

SettingsDialog::~SettingsDialog()
{
    if (m_observerId)
        ConfigManager::instance().removeObserver(m_observerId);
}

QScrollArea *SettingsDialog::contentArea() const
{
    return m_contentArea;
}

SettingsGroup *SettingsDialog::addGroup(const QString &title)
{
    SettingsGroup *group = new SettingsGroup(title, this);
    m_contentLayout->addWidget(group);
    return group;
}

void SettingsDialog::addItemToGroup(SettingsGroup *group, SettingsItemWidget *item)
{
    group->card()->addItem(item);
    m_allItems.append(item);
}

void SettingsDialog::onApply()
{
    for (auto *item : m_allItems)
        item->apply();

    ConfigManager::instance().save();
    updateButtonStates();
}

void SettingsDialog::onOk()
{
    onApply();
    accept();
}

void SettingsDialog::onCancel()
{
    close();
}

void SettingsDialog::onItemValueChanged()
{
    if (m_initializing)
        return;

    auto *item = qobject_cast<SettingsItemWidget *>(sender());
    if (!item || item->settingsKey().isEmpty())
        return;

    syncItemToConfig(item);
}

void SettingsDialog::onDirtyStateChanged(bool dirty)
{
    m_applyBtn->setEnabled(dirty);
    m_okBtn->setEnabled(dirty);
}

void SettingsDialog::updateButtonStates()
{
    bool dirty = ConfigManager::instance().hasChanges();
    m_applyBtn->setEnabled(dirty);
    m_okBtn->setEnabled(dirty);
}

void SettingsDialog::syncItemToConfig(SettingsItemWidget *item)
{
    auto *mgr = &ConfigManager::instance();
    const QString &key = item->settingsKey();

    if (auto *toggle = qobject_cast<SettingsToggleItem *>(item)) {
        mgr->setValue(key, toggle->isChecked());
    } else if (auto *choice = qobject_cast<SettingsChoiceItem *>(item)) {
        mgr->setValue(key, choice->currentChoice());
    } else if (auto *input = qobject_cast<SettingsInputItem *>(item)) {
        mgr->setValue(key, input->text());
    }
}

void SettingsDialog::loadConfigToItems()
{
    m_initializing = true;

    auto *mgr = &ConfigManager::instance();
    mgr->load({});

    for (auto *item : m_allItems) {
        const QString &key = item->settingsKey();
        if (key.isEmpty())
            continue;

        QVariant val = mgr->value(key);

        if (auto *toggle = qobject_cast<SettingsToggleItem *>(item)) {
            if (val.isValid()) {
                toggle->setChecked(val.toBool());
            } else {
                mgr->setValue(key, toggle->isChecked());
            }
        } else if (auto *choice = qobject_cast<SettingsChoiceItem *>(item)) {
            if (val.isValid()) {
                choice->setCurrentChoice(val.toString());
            } else {
                mgr->setValue(key, choice->currentChoice());
            }
        } else if (auto *input = qobject_cast<SettingsInputItem *>(item)) {
            if (val.isValid()) {
                input->setText(val.toString());
            } else {
                mgr->setValue(key, input->text());
            }
        }
    }

    mgr->takeSnapshot();
    m_initializing = false;
    updateButtonStates();
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

    m_scrollContent = new QWidget();
    m_scrollContent->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(m_scrollContent);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(18);
    m_contentLayout->addStretch();
    m_contentArea->setWidget(m_scrollContent);

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
        "}"
        "QPushButton:disabled {"
        "  color: #aaaaaa;"
        "  background: #f5f5f5;"
        "  border-color: #d8d8d8;"
        "}";

    m_applyBtn = new QPushButton(tr("Apple"), this);
    m_okBtn = new QPushButton(tr("OK"), this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);

    m_applyBtn->setStyleSheet(btnStyle);
    m_okBtn->setStyleSheet(btnStyle);
    m_cancelBtn->setStyleSheet(btnStyle);

    m_applyBtn->setEnabled(false);
    m_okBtn->setEnabled(false);

    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_okBtn, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);

    btnLayout->addStretch();
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

void SettingsDialog::buildContent()
{
    setStyleSheet("QDialog { background: #f3f3f3; }");
    SettingsContentBuilder::build(this);

    for (auto *item : m_allItems) {
        connect(item, &SettingsItemWidget::valueChanged,
                this, &SettingsDialog::onItemValueChanged);

        auto *navItem = qobject_cast<SettingsNavigateItem *>(item);
        if (!navItem)
            continue;

        const QString &target = navItem->target();
        if (target == "font") {
            connect(navItem, &SettingsNavigateItem::clicked, this, [this]() {
                FontSettingsDialog dlg(this);
                dlg.exec();
            });
        } else if (target == "opacity") {
            connect(navItem, &SettingsNavigateItem::clicked, this, [this]() {
                OpacityDialog dlg(this);
                dlg.exec();
            });
        }
    }

    SettingsChoiceItem *captureSourceItem = nullptr;
    SettingsItemWidget *pidItem = nullptr;
    for (auto *item : m_allItems) {
        if (item->settingsKey() == "audio.capture_source")
            captureSourceItem = qobject_cast<SettingsChoiceItem *>(item);
        else if (item->settingsKey() == "audio.target_pid")
            pidItem = item;
    }

    if (captureSourceItem && pidItem) {
        auto updatePidVisibility = [captureSourceItem, pidItem]() {
            pidItem->setVisible(captureSourceItem->currentChoice() == "Process audio");
        };
        updatePidVisibility();
        connect(captureSourceItem, &SettingsItemWidget::valueChanged,
                this, updatePidVisibility);
    }

    loadConfigToItems();

    m_observerId = ConfigManager::instance().addObserver(
        [this](bool dirty) { onDirtyStateChanged(dirty); });
}
