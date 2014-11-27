#ifndef QCLITEST_H
#define QCLITEST_H

#include <QtTest>
#include <qcli.h>

using namespace QCli;

class QCliTest : public QObject
{
    Q_OBJECT

protected:
    CommandLineParser *parser;

private slots:
    virtual void init();
    virtual void cleanup();
};

// Macros to make testing easier.
#define D(k, a) \
    struct k { \
        static void func(CommandLineParser *parser, \
                  CommandLineParser::ParsingResult result, \
                  const QString &name, QVariant value, bool *ok) { \
            Q_UNUSED(parser);Q_UNUSED(result);Q_UNUSED(name);Q_UNUSED(value); \
            Q_UNUSED(ok); a }}

#define CB(k) (&k::func)

#define ARGS (QStringList() << "_cmd")

#endif // QCLITEST_H

