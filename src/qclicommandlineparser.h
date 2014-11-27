#ifndef QCLICOMMANDLINEPARSER_H
#define QCLICOMMANDLINEPARSER_H

#include <QObject>
#include <QMetaType>
#include "qcli_global.h"
#include "qclioption.h"

namespace QCli
{

class Settings;
class CommandLineParserPrivate;

class QCLIISHARED_EXPORT CommandLineParser : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(CommandLineParser)
    CommandLineParserPrivate * const d_ptr;

public:

    enum ParsingResult
    {
        OptionFound,
        ArgumentFound,
        GroupMismatch,
        ValueMissing,
        OptionUnknown,
    };
    Q_ENUMS(ParsingResult)

    typedef void (*ParsingCallback)(
            CommandLineParser *parser, CommandLineParser::ParsingResult result,
            const QString &name, QVariant value, bool *stop);

    explicit CommandLineParser(QObject *parent = 0);
    ~CommandLineParser();

    void beginOptionGroup(const QString &name);
    void endOptionGroup();

    void addOption(const QString &name, const QChar &alias,
                   OptionFlags flags = OptionValueNone);
    void addOption(const QString &name, OptionFlags flags = OptionValueNone);

    bool parse(const QList<QString> &arguments,
               QObject *obj, const char *callback);
    bool parse(int argc, char *argv[], QObject *obj, const char *callback);
    bool parse(QObject *obj, const char *callback);

    bool parse(const QList<QString> &arguments, ParsingCallback callback = 0);
    bool parse(int argc, char *argv[], ParsingCallback callback = 0);
    bool parse(ParsingCallback callback = 0);

    Settings *settings() const;
    void setSettings(Settings *s);

    QString currentGroupName() const;

    void redirectStdOut(QIODevice *device);
    void redirectStdErr(QIODevice *device);

    QIODevice *stdOut() const;
    QIODevice *stdErr() const;
};

}   // namespace QCli

Q_DECLARE_METATYPE(QCli::CommandLineParser::ParsingResult)

#endif // QCLICOMMANDLINEPARSER_H

