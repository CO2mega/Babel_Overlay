#pragma once

#include <QString>
#include <QObject>
#include <QVariantMap>

class ISettingsItem
{
public:
    virtual ~ISettingsItem() = default;

    virtual QString settingsKey() const { return {}; }
    virtual void apply() = 0;
    virtual void reset() = 0;
    virtual bool isModified() const { return false; }
    virtual void configure(const QVariantMap &props) { Q_UNUSED(props); }
};
