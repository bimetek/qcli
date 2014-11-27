#include "qclisettings.h"
#include <QSet>
#include <QSettings>
#include <QStringList>
#include "qclioption.h"

namespace QCli
{

static QString ArgumentsKey = "38f9b7b0-755f-11e4-82f8-0800200c9a66";

class SettingsPrivate
{
    Q_DECLARE_PUBLIC(Settings)
    Settings * const q_ptr;

public:
    SettingsPrivate(Settings *q, const QString &name,
                    Settings *parentSettings = 0) :
        q_ptr(q), name(name), parentSettings(parentSettings)
    {
        q->registerArray(ArgumentsKey);
    }

    QString name;
    Settings *parentSettings;
    QHash<QString, QVariant> values;
    QSet<QString> arrayKeys;
};

Settings::Settings(const QString &name, Settings *parent) :
    QObject(parent), d_ptr(new SettingsPrivate(this, name, parent))
{
}

Settings::Settings(const QString &name, QObject *parent) :
    QObject(parent), d_ptr(new SettingsPrivate(this, name))
{
}

Settings::~Settings()
{
    delete d_ptr;
}

Settings *Settings::parentSettings() const
{
    return d_ptr->parentSettings;
}

void Settings::load(QSettings *settings)
{
    Q_D(Settings);
    d->values.clear();
    foreach (QString key, settings->allKeys())
    {
        QVariant value = settings->value(key);
        if (value.type() == QVariant::Hash)
        {
            QHash<QString, QVariant> hash = value.toHash();
            typedef QHash<QString, QVariant>::const_iterator Iter;
            for (Iter it = hash.constBegin(); it != hash.constEnd(); it++)
            {
                QString key = it.key();
                if (key.startsWith(OptionNamePrefix))
                    key = key.mid(OptionNamePrefix.size());
                else if (key.startsWith(OptionAliasPrefix))
                    key = key.mid(OptionAliasPrefix.size());
                setValue(key, it.value());
            }
        }
        else
        {
            if (key.startsWith(OptionNamePrefix))
                key = key.mid(OptionNamePrefix.size());
            else if (key.startsWith(OptionAliasPrefix))
                key = key.mid(OptionAliasPrefix.size());
            setValue(key, value);
        }
    }
}

void Settings::save(QSettings *settings) const
{
    typedef QHash<QString, QVariant>::const_iterator Iter;
    for (Iter it = d_ptr->values.constBegin();
            it != d_ptr->values.constEnd(); it++)
        settings->setValue(it.key(), it.value());
    settings->sync();
}

QVariant Settings::value(const QString &key) const
{
    if (!d_ptr->arrayKeys.contains(key))
        return settings(key)->localValue(key);
    QList<QVariant> values;
    for (const Settings *p = this; p; p = p->parentSettings())
        values.append(p->localValue(key).toList());
    return values;
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    Q_D(Settings);
    if (!d->arrayKeys.contains(key))
    {
        setLocalValue(key, value);
        return;
    }
    QVariant var = d->values.value(key);
    QList<QVariant> list = var.toList();
    if (var.type() != QVariant::List)
    {
        if (!var.isNull())
            list.append(var);
        setLocalValue(key, list);
    }
    if (value.type() == QVariant::List)
        list.append(value.toList());
    else
        list.append(value);
}

void Settings::registerArray(const QString &key)
{
    Q_D(Settings);
    d->arrayKeys.insert(key);
}

QVariant Settings::localValue(const QString &key) const
{
    return d_ptr->values.value(key);
}

void Settings::setLocalValue(const QString &key, const QVariant &value)
{
    Q_D(Settings);
    d->values.insert(key, value);
}

Settings *Settings::settings(const QString &value, const QString &key) const
{
    for (Settings *p = const_cast<Settings *>(this); p; p = p->parentSettings())
    {
        QList<QVariant> arguments = localValue(key).toList();
        foreach (const QVariant &v, arguments)
        {
            if (v.type() == QVariant::String && v.toString() == value)
                return p;
        }
    }
    return 0;
}

Settings *Settings::settings(const QString &key) const
{
    for (Settings *p = const_cast<Settings *>(this); p; p = p->parentSettings())
    {
        if (p->d_ptr->values.contains(key))
            return p;
    }
    return 0;
}

void Settings::addArgument(const QString &argument)
{
    setValue(ArgumentsKey, argument);
}

}   // namespace QCli
