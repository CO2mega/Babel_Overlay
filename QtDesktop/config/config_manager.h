#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QByteArray>
#include <QVector>
#include <functional>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager &instance();

    bool load(const QString &path);
    bool save();
    bool saveAs(const QString &path);

    QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    void setValue(const QString &key, const QVariant &value);
    QStringList keys() const;

    QByteArray computeHash() const;
    void takeSnapshot();
    bool hasChanges() const;

    using Observer = std::function<void(bool dirty)>;
    int addObserver(Observer observer);
    void removeObserver(int id);

signals:
    void dirtyStateChanged(bool dirty);

private:
    ConfigManager();

    QString m_filePath;
    QVariantMap m_values;
    QByteArray m_snapshotHash;
    int m_nextObserverId = 1;
    QVector<QPair<int, Observer>> m_observers;

    QString defaultConfigPath() const;
    void notifyObservers();
};

#endif // CONFIG_MANAGER_H
