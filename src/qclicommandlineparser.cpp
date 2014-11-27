#include "qclicommandlineparser.h"
#include <cstdio>
#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include "qclisettings.h"


namespace QCli
{

namespace
{
enum Lookup
{
    OptionFound = 0,
    EndOfOptionsFound,
    GroupNameFound,
    ArgumentFound,
    LookupFailed,
};
}

static void simpleParsingCallback(
        CommandLineParser *parser, CommandLineParser::ParsingResult result,
        const QString &name, QVariant value, bool *stop);

class Option : public QObject
{
public:
    Option(const QString name, const QString &alias, OptionFlags flags,
           QObject *parent) :
        QObject(parent), name(name), alias(alias), flags(flags) {}

    Option(const QString name, OptionFlags flags, QObject *parent) :
        QObject(parent), name(name), alias(), flags(flags) {}

    QString name;
    QString alias;

    OptionFlags flags;
};

class Group : public QSet<Option *>
{
public:
    Group(const QString &name) : QSet<Option *>(), name(name) {}

    inline void addOption(Option *option)
    {
        insert(option);
    }
    inline bool hasOption(Option *option)
    {
        return contains(option);
    }

    QString name;
};

struct OptionResult
{
    OptionResult() : option(0), valueString(), lookup(OptionFound) {}
    Option *option;
    QString valueString;
    Lookup lookup;
};

struct CallbackInvoker
{
    CallbackInvoker(CommandLineParser *parser,
                    CommandLineParser::ParsingCallback func) :
        parser(parser), func(func), method(0, 0)
    {
        if (!func)
            func = &simpleParsingCallback;
    }

    CallbackInvoker(CommandLineParser *parser,
                    QObject *obj, const char *callback) :
        parser(parser), func(0), method(obj, callback)
    {
#ifndef QT_NO_DEBUG
        // Make sure the method signature is correct.
        QString sig = QString("%1(QCli::CommandLineParser*,"
                              "QCli::CommandLineParser::ParsingResult,"
                              "QString,QVariant,bool*)").arg(callback);
        Q_ASSERT(obj->metaObject()->indexOfMethod(qPrintable(sig)) >= 0);
#endif
    }

    void invoke(CommandLineParser::ParsingResult result,
                const QString &name, QVariant value, bool *stop) const
    {
        if (!func)
        {
            bool ok = QMetaObject::invokeMethod(
                        method.obj, method.callback,
                        Q_ARG(QCli::CommandLineParser *, parser),
                        Q_ARG(QCli::CommandLineParser::ParsingResult, result),
                        Q_ARG(QString, name), Q_ARG(QVariant, value),
                        Q_ARG(bool *, stop));
#ifndef QT_NO_DEBUG
            Q_ASSERT(ok);
#else
            Q_UNUSED(ok);
#endif
            return;
        }
        func(parser, result, name, value, stop);
    }

    CommandLineParser *parser;
    CommandLineParser::ParsingCallback func;
    struct Method {
        Method(QObject *obj, const char *callback) :
            obj(obj), callback(callback) {}
        QObject *obj;
        const char *callback;
    } method;
};

class CommandLineParserPrivate
{
    Q_DECLARE_PUBLIC(CommandLineParser)
    CommandLineParser * const q_ptr;

public:
    CommandLineParserPrivate(CommandLineParser *q);
    ~CommandLineParserPrivate();

    OptionResult findOption(const QString &optionString);
    inline bool isGroupName(const QString &optionString);
    inline bool isOptionNameLike(const QString &optionString);
    inline void insertOption(const QString &key, Option *option);

    bool parse(const QStringList &arguments, const CallbackInvoker &invoker);

    inline bool booleanize(const QString &str);

    QHash<QString, Option *> options;
    QHash<QString, Group *> groups;
    Settings *settings;

    Group *currentGroup;

    QHash<QString, QVariant> parsedOptions;
    QList<QVariant> parsedArguments;

    QIODevice *outDevice;
    QIODevice *errDevice;
};

CommandLineParserPrivate::CommandLineParserPrivate(CommandLineParser *q) :
    q_ptr(q), currentGroup(0), outDevice(0), errDevice(0)
{
    QFile *outFile = new QFile();
    outFile->open(stdout, QIODevice::WriteOnly);
    outDevice = outFile;

    QFile *errFile = new QFile();
    errFile->open(stderr, QIODevice::WriteOnly);
    errDevice = errFile;
}

CommandLineParserPrivate::~CommandLineParserPrivate()
{
    qDeleteAll(groups);
    delete outDevice;
    delete errDevice;
}

OptionResult CommandLineParserPrivate::findOption(const QString &optionString)
{
    OptionResult result;
    if (optionString.startsWith(OptionNamePrefix))
    {
        if (optionString.size() == 2)
        {
            result.lookup = EndOfOptionsFound;
            return result;
        }
    }
    else if (optionString.startsWith(OptionAliasPrefix))
    {
        if (optionString.size() == 1)
        {
            result.lookup = EndOfOptionsFound;
            return result;
        }
    }
    else if (isGroupName(optionString))
    {
        currentGroup = groups.value(optionString);
        result.lookup = GroupNameFound;
        return result;
    }
    else
    {
        result.lookup = ArgumentFound;
        result.valueString = optionString;
        return result;
    }

    // Find first occurance of '=' (not last; we can control the option name,
    // but should allow the user to use the equal sign in value inputs).
    int equalSignLocation = optionString.indexOf('=');
    if (equalSignLocation != -1)
    {
        if (equalSignLocation + 1 < optionString.size())
            result.valueString = optionString.mid(equalSignLocation + 1);
        else
            result.valueString = "";
        result.option = options.value(optionString.left(equalSignLocation));
    }
    else
    {
        result.option = options.value(optionString);
    }
    result.lookup = result.option ? OptionFound : LookupFailed;
    return result;
}

bool CommandLineParserPrivate::isGroupName(const QString &optionString)
{
    if (groups.contains(optionString))
        return true;
    return false;
}

bool CommandLineParserPrivate::isOptionNameLike(const QString &optionString)
{
    if (optionString.startsWith(OptionNamePrefix))
        return true;
    if (optionString.startsWith(OptionAliasPrefix))
        return true;
    return false;
}

void CommandLineParserPrivate::insertOption(const QString &key, Option *option)
{
    if (options.contains(key))
    {
        QTextStream err(errDevice);
        err << "Replacing existing option " << key << "!" << endl;
    }
    options.insert(key, option);
}

bool CommandLineParserPrivate::parse(
        const QStringList &arguments, const CallbackInvoker &invoker)
{
    Q_ASSERT(arguments.size() > 0);

    currentGroup = 0;
    parsedOptions.clear();
    parsedArguments.clear();

    bool success = true;
    bool stop = false;
    CommandLineParser::ParsingResult parsingResult;

    // Skips the first argument (which is the command name).
    QStringList::const_iterator it = arguments.constBegin() + 1;
    while (it < arguments.constEnd())
    {
        OptionResult result = findOption(*it);
        switch (result.lookup)
        {
        // End of options detected. Skip this one and end option parsing.
        case EndOfOptionsFound:
            it++;
            stop = true;
            break;
        // This is a group name. Continue with next.
        case GroupNameFound:
            invoker.invoke(CommandLineParser::OptionFound, *it,
                           QVariant(true), &stop);
            it++;
            continue;
        default:
            break;
        }
        if (stop)
            break;

        QString name;
        QVariant value = result.valueString;
        parsingResult = CommandLineParser::OptionFound;

        switch (result.lookup)
        {
        case LookupFailed:      // Is option-like, but not a known option.
            name = *it;
            parsingResult = CommandLineParser::OptionUnknown;
            success = false;
            break;

        case ArgumentFound:     // Is not option-like.
            value = *it;
            parsingResult = CommandLineParser::ArgumentFound;
            parsedArguments.append(value);
            break;

        default:
            Q_ASSERT(result.option);

            // Is an option, but not found in current group.
            if (currentGroup && !currentGroup->hasOption(result.option))
            {
                name = *it;
                parsingResult = CommandLineParser::GroupMismatch;
                success = false;
            }
            // Lookup successful. Parse!
            else
            {
                // Note that we always report positive option name (we only use
                // negative form internally)!
                name = result.option->name;

                // Option is a negative boolean "switch": If we already have a
                // value (via --name=value syntax), convert it to inverted
                // boolean, otherwise return false.
                QString neg = QString("%1no-%2").arg(OptionNamePrefix, name);
                if ((*it).startsWith(neg))
                {
                    if (result.valueString.isNull())
                        value = false;
                    else
                        value = !booleanize(result.valueString);
                }

                // After clearing the negative switch, this should be one of
                // OptionSwitch, OptionValueRequired, or OptionValueOptional.
                switch ((int)result.option->flags & ~OptionNegativeSwitch)
                {
                case OptionValueRequired:
                    // Check next option. If it "looks like" an option (i.e.
                    // starts with -- or - or is one of option group names),
                    // notify about missing value. Also notify about missing
                    // value if this is the last option. If we already have the
                    // value (via --name=value syntax), no need to search.
                    if (value.isNull())
                    {
                        if (it + 1 < arguments.constEnd())
                        {
                            const QString &next = *(it + 1);
                            if (isOptionNameLike(next) || isGroupName(next))
                            {
                                parsingResult = CommandLineParser::ValueMissing;
                            }
                            else
                            {
                                it++;
                                value = next;
                            }
                        }
                        else
                        {
                            parsingResult = CommandLineParser::ValueMissing;
                        }
                    }
                    break;
                case OptionValueOptional:
                    // Options can have optional value: Check next option. If it
                    // "looks like" a value (i.e. doesn't start with -- or -),
                    // use it. Otherwie assume true (the same if there's no more
                    // option). If we already have the value (via --name=value
                    // syntax), no need to search.
                    if (value.isNull())
                    {
                        if (it + 1 < arguments.constEnd())
                        {
                            const QString &next = *(it + 1);
                            if (isOptionNameLike(next) || isGroupName(next))
                            {
                                value = true;
                            }
                            else
                            {
                                it++;
                                value = next;
                            }
                        }
                        else
                        {
                            value = true;
                        }
                    }
                    break;
                case OptionSwitch:
                    // Option is a negative boolean "switch": If we already have
                    // a value (via --name=value syntax), convert it to boolean,
                    // otherwise return true.
                    if (!value.isNull())            // There is a value already.
                        value = value.toBool();     // Normalize to boolean.
                    else if (result.valueString.isNull())
                        value = true;
                    else
                        value = !booleanize(result.valueString);
                    break;
                default:
                    parsingResult = CommandLineParser::OptionUnknown;
                    break;
                }
            }
            break;
        }

        // Prepare remaining parameters and notify observer. If observer stops
        // the operation, quit immediately.
        invoker.invoke(parsingResult, name, value, &stop);
        if (stop)
            return false;

        // Remember parsed option and continue with next one.
        if (!name.isNull() && !value.isNull())
            parsedOptions.insert(name, value);
        it++;
    }

    // Prepare arguments (arguments are command line options after options).
    parsingResult = CommandLineParser::ArgumentFound;
    for (; it < arguments.constEnd(); it++)
    {
        QVariant value = *it;
        bool stop = false;
        parsedArguments.append(value);
        invoker.invoke(parsingResult, QString(), value, &stop);
        if (stop)
            return false;
    }
    return success;
}

bool CommandLineParserPrivate::booleanize(const QString &str)
{
    QString stripped = str.trimmed();
    bool ok = false;

    // Try integer parsing.
    qlonglong v = stripped.toLongLong(&ok);
    if (ok)
        return v;

    // Try boolean parsing.
    if (stripped.compare("false", Qt::CaseInsensitive) == 0)
        return false;

    // Judge by string length.
    return stripped.size();
}


CommandLineParser::CommandLineParser(QObject *parent) :
    QObject(parent), d_ptr(new CommandLineParserPrivate(this))
{
    qRegisterMetaType<QCli::CommandLineParser::ParsingResult>
            ("QCli::CommandLineParser::ParsingResult");
}

CommandLineParser::~CommandLineParser()
{
    delete d_ptr;
}

void CommandLineParser::beginOptionGroup(const QString &name)
{
    Q_D(CommandLineParser);

    Group *existed = d->groups.value(name);
    if (existed)
    {
        QTextStream err(d->errDevice);
        err << "Group " << name << " is already registered!" << endl;
        d->currentGroup = existed;
        return;
    }

    d->currentGroup = new Group(name);
    d->groups.insert(name, d->currentGroup);
}

void CommandLineParser::endOptionGroup()
{
    Q_D(CommandLineParser);
    d->currentGroup = 0;
}

void CommandLineParser::addOption(
        const QString &name, const QChar &alias, OptionFlags flags)
{
    Q_D(CommandLineParser);
    Option *option = new Option(name, alias, flags, this);
    d->insertOption(QString("%1%2").arg(OptionNamePrefix, name), option);
    if (d->currentGroup)
        d->currentGroup->addOption(option);

    if (!alias.isNull())
    {
        QString key = QString("%1%2").arg(OptionAliasPrefix, alias);
        d->insertOption(key, option);
    }

    if (flags & OptionNegativeSwitch)
    {
        Option *negativeOption = new Option(name, OptionSwitch, this);
        d->insertOption(QString("%1no-%2").arg(OptionNamePrefix, name),
                        negativeOption);
        if (d->currentGroup)
            d->currentGroup->addOption(negativeOption);
    }
}

void CommandLineParser::addOption(const QString &name, OptionFlags flags)
{
    addOption(name, QChar(), flags);
}

bool CommandLineParser::parse(
        const QList<QString> &arguments, QObject *obj, const char *callback)
{
    Q_D(CommandLineParser);
    return d->parse(arguments, CallbackInvoker(this, obj, callback));
}

bool CommandLineParser::parse(
        int argc, char *argv[], QObject *obj, const char *callback)
{
    QStringList arguments;
    for (int i = 0; i < argc; i++)
        arguments.append(QString::fromLocal8Bit(argv[i]));
    return parse(arguments, obj, callback);
}

bool CommandLineParser::parse(QObject *obj, const char *callback)
{
    return parse(qApp->arguments(), obj, callback);
}

bool CommandLineParser::parse(
        const QList<QString> &arguments, ParsingCallback callback)
{
    // TODO: If callback is null, use default.
    Q_D(CommandLineParser);
    return d->parse(arguments, CallbackInvoker(this, callback));
}

bool CommandLineParser::parse(int argc, char *argv[], ParsingCallback callback)
{
    QStringList arguments;
    for (int i = 0; i < argc; i++)
        arguments.append(QString::fromLocal8Bit(argv[i]));
    return parse(arguments, callback);
}

bool CommandLineParser::parse(ParsingCallback callback)
{
    return parse(qApp->arguments(), callback);
}

Settings *CommandLineParser::settings() const
{
    return d_ptr->settings;
}

void CommandLineParser::setSettings(Settings *s)
{
    Q_D(CommandLineParser);
    d->settings = s;
}

QString CommandLineParser::currentGroupName() const
{
    if (!d_ptr->currentGroup)
        return QString();
    return d_ptr->currentGroup->name;
}

void CommandLineParser::redirectStdOut(QIODevice *device)
{
    Q_D(CommandLineParser);
    delete d->outDevice;
    d->outDevice = device;
}

void CommandLineParser::redirectStdErr(QIODevice *device)
{
    Q_D(CommandLineParser);
    delete d->errDevice;
    d->errDevice = device;
}

QIODevice *CommandLineParser::stdOut() const
{
    return d_ptr->outDevice;
}

QIODevice *CommandLineParser::stdErr() const
{
    return d_ptr->errDevice;
}


static void simpleParsingCallback(
        CommandLineParser *parser, CommandLineParser::ParsingResult result,
        const QString &name, QVariant value, bool *)
{
    QTextStream err(parser->stdErr());
    switch (result)
    {
    case CommandLineParser::OptionUnknown:
        err << "Unknown command line option " << name << ", try --help!";
        break;
    case CommandLineParser::ValueMissing:
        err << "Missing value for command line option " << name <<
               ", try --help!";
        break;
    case CommandLineParser::GroupMismatch:
        err << "Invalid option " << name << " for group " <<
               parser->currentGroupName() << ", try --help!";
        break;
    case CommandLineParser::ArgumentFound:
        parser->settings()->addArgument(value.toString());
        break;
    case CommandLineParser::OptionFound:
        parser->settings()->setValue(name, value);
    }
}

}   // namespace QCli
