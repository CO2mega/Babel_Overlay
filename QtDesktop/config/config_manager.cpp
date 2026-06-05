#include "config_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <fstream>
#include <toml++/toml.hpp>

ConfigManager &ConfigManager::instance()
{
    static ConfigManager mgr;
    return mgr;
}

ConfigManager::ConfigManager()
    : QObject(nullptr)
{
}

QString ConfigManager::defaultConfigPath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/config.toml";
}

bool ConfigManager::load(const QString &path)
{
    m_filePath = path.isEmpty() ? defaultConfigPath() : path;

    try {
        auto tbl = toml::parse_file(m_filePath.toStdString());

        m_values.clear();

        for (const auto &[sectionKey, sectionVal] : tbl) {
            if (!sectionVal.is_table())
                continue;

            std::string section = std::string(sectionKey);
            const auto &sub = *sectionVal.as_table();

            for (const auto &[key, val] : sub) {
                std::string fullKey = section + "." + std::string(key);

                if (val.is_boolean()) {
                    m_values[QString::fromStdString(fullKey)] = val.as_boolean()->get();
                } else if (val.is_integer()) {
                    m_values[QString::fromStdString(fullKey)] =
                        static_cast<qint64>(val.as_integer()->get());
                } else if (val.is_floating_point()) {
                    m_values[QString::fromStdString(fullKey)] = val.as_floating_point()->get();
                } else if (val.is_string()) {
                    m_values[QString::fromStdString(fullKey)] =
                        QString::fromStdString(val.as_string()->get());
                }
            }
        }
    } catch (const toml::parse_error &err) {
        qWarning("ConfigManager: parse error: %s", err.what());
        return false;
    }

    takeSnapshot();
    return true;
}

bool ConfigManager::save()
{
    if (m_filePath.isEmpty())
        m_filePath = defaultConfigPath();
    return saveAs(m_filePath);
}

bool ConfigManager::saveAs(const QString &path)
{
    toml::table root;

    // Group keys by section
    QHash<QString, toml::table *> sections;
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        QString fullKey = it.key();
        int dotPos = fullKey.indexOf('.');
        if (dotPos < 0)
            continue;

        QString section = fullKey.left(dotPos);
        QString key = fullKey.mid(dotPos + 1);

        toml::table *secTable = sections.value(section);
        if (!secTable) {
            root.insert(section.toStdString(), toml::table{});
            secTable = root[section.toStdString()].as_table();
            sections[section] = secTable;
        }

        QVariant v = it.value();
        switch (v.typeId()) {
        case QMetaType::Bool:
            secTable->insert(key.toStdString(), v.toBool());
            break;
        case QMetaType::Int:
        case QMetaType::LongLong:
            secTable->insert(key.toStdString(), v.toLongLong());
            break;
        case QMetaType::Double:
            secTable->insert(key.toStdString(), v.toDouble());
            break;
        default:
            secTable->insert(key.toStdString(), v.toString().toStdString());
            break;
        }
    }

    try {
        QFileInfo fi(path);
        QDir().mkpath(fi.absolutePath());

        std::ofstream ofs(path.toStdString());
        if (!ofs.is_open())
            return false;

        ofs << root;
        ofs.close();

        m_filePath = path;
        takeSnapshot();
        return true;
    } catch (const std::exception &e) {
        qWarning("ConfigManager: save error: %s", e.what());
        return false;
    }
}

QVariant ConfigManager::value(const QString &key, const QVariant &defaultValue) const
{
    return m_values.value(key, defaultValue);
}

void ConfigManager::setValue(const QString &key, const QVariant &value)
{
    m_values[key] = value;
    notifyObservers();
}

QStringList ConfigManager::keys() const
{
    return m_values.keys();
}

QByteArray ConfigManager::computeHash() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    QStringList sortedKeys = m_values.keys();
    sortedKeys.sort();
    for (const QString &key : sortedKeys) {
        stream << key << m_values[key];
    }

    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

void ConfigManager::takeSnapshot()
{
    m_snapshotHash = computeHash();
    notifyObservers();
}

bool ConfigManager::hasChanges() const
{
    return computeHash() != m_snapshotHash;
}

int ConfigManager::addObserver(Observer observer)
{
    int id = m_nextObserverId++;
    m_observers.append({id, std::move(observer)});
    return id;
}

void ConfigManager::removeObserver(int id)
{
    m_observers.erase(
        std::remove_if(m_observers.begin(), m_observers.end(),
                       [id](const auto &p) { return p.first == id; }),
        m_observers.end());
}

void ConfigManager::notifyObservers()
{
    bool dirty = hasChanges();
    for (const auto &[id, obs] : m_observers) {
        obs(dirty);
    }
    emit dirtyStateChanged(dirty);
}
