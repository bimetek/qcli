#ifndef QCLISETTINGS_H
#define QCLISETTINGS_H

#include <QObject>
#include "qcli_global.h"
class QSettings;

namespace QCli
{

class SettingsPrivate;

class QCLIISHARED_EXPORT Settings : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Settings)
    SettingsPrivate * const d_ptr;

public:
    Settings(const QString &name, Settings *parent);
    Settings(const QString &name, QObject *parent = 0);
    ~Settings();

    QString name() const;
    Settings *parentSettings() const;

    void load(QSettings *settings);
    void save(QSettings *settings) const;

    QVariant value(const QString &key) const;
    void setValue(const QString &key, const QVariant &value);

    void registerArray(const QString &key);
    QVariant localValue(const QString &key) const;
    void setLocalValue(const QString &key, const QVariant &value);

    Settings *settings(const QString &value, const QString &key) const;
    Settings *settings(const QString &key) const;

    void addArgument(const QString &argument);
};

}   // namespace QCli

#endif // QCLISETTINGS_H

