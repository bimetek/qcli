#include "simpletest.h"

void SimpleTest::testRequired()
{
    D(ValueMissing, {
          QCOMPARE(result, CommandLineParser::ValueMissing);
          QCOMPARE(name, QString("aaa"));
      });
    D(OptionFound, {
          QCOMPARE(result, CommandLineParser::OptionFound);
          QCOMPARE(name, QString("aaa"));
          QCOMPARE(value, QVariant("foo"));
      });
    D(OptionUnknown, {
          QCOMPARE(result, CommandLineParser::OptionUnknown);
      });

    parser->addOption("aaa", 'a', OptionValueRequired);

    // Long option name.
    parser->parse(ARGS << "--aaa", CB(ValueMissing));
    parser->parse(ARGS << "--aaa=foo", CB(OptionFound));
    parser->parse(ARGS << "--aaa" << "foo", CB(OptionFound));

    // Option alias.
    parser->parse(ARGS << "-a", CB(ValueMissing));
    parser->parse(ARGS << "-a=foo", CB(OptionFound));
    parser->parse(ARGS << "-a" << "foo", CB(OptionFound));

    // Invalid forms.
    parser->parse(ARGS << "-aaa", CB(OptionUnknown));
    parser->parse(ARGS << "-aaa=foo", CB(OptionUnknown));
    parser->parse(ARGS << "--a", CB(OptionUnknown));
    parser->parse(ARGS << "--a=foo", CB(OptionUnknown));
}

void SimpleTest::testOptional()
{
    D(OptionDefault, {
          QCOMPARE(result, CommandLineParser::OptionFound);
          QCOMPARE(name, QString("aaa"));
          QCOMPARE(value, QVariant(true));
      });
    D(OptionFound, {
          QCOMPARE(result, CommandLineParser::OptionFound);
          QCOMPARE(name, QString("aaa"));
          QCOMPARE(value, QVariant("foo"));
      });
    D(OptionUnknown, {
          QCOMPARE(result, CommandLineParser::OptionUnknown);
      });

    parser->addOption("aaa", 'a', OptionValueOptional);

    // Long option name.
    parser->parse(ARGS << "--aaa", CB(OptionDefault));
    parser->parse(ARGS << "--aaa=foo", CB(OptionFound));
    parser->parse(ARGS << "--aaa" << "foo", CB(OptionFound));

    // Option alias.
    parser->parse(ARGS << "-a", CB(OptionDefault));
    parser->parse(ARGS << "-a=foo", CB(OptionFound));
    parser->parse(ARGS << "-a" << "foo", CB(OptionFound));

    // Invalid forms.
    parser->parse(ARGS << "-aaa", CB(OptionUnknown));
    parser->parse(ARGS << "-aaa=foo", CB(OptionUnknown));
    parser->parse(ARGS << "--a", CB(OptionUnknown));
    parser->parse(ARGS << "--a=foo", CB(OptionUnknown));
}

void SimpleTest::testSwitch()
{
    D(OptionFoundTrue, {
          QCOMPARE(result, CommandLineParser::OptionFound);
          QCOMPARE(name, QString("aaa"));
          QCOMPARE(value, QVariant(true));
      });
    D(OptionFoundFalse, {
          QCOMPARE(result, CommandLineParser::OptionFound);
          QCOMPARE(name, QString("aaa"));
          QCOMPARE(value, QVariant(false));
      });
    D(Argument, {
          QList<QString> possibleNames;
          possibleNames.append("aaa");
          possibleNames.append(QString());

          QVERIFY(possibleNames.contains(name));

          if (name == "aaa")
          {
              QCOMPARE(result, CommandLineParser::OptionFound);
              QCOMPARE(value, QVariant(true));
          }
          else if (name.isNull())
          {
              QCOMPARE(result, CommandLineParser::ArgumentFound);
              QCOMPARE(value, QVariant("foo"));
          }
          else
          {
              QString msg = QString("Unrecignized name %1").arg(name);
              QFAIL(qPrintable(msg));
          }
      });
    D(OptionUnknown, {
          QCOMPARE(result, CommandLineParser::OptionUnknown);
      });

    parser->addOption("aaa", 'a', OptionSwitch);

    // Valid inputs.
    parser->parse(ARGS << "--aaa", CB(OptionFoundTrue));
    parser->parse(ARGS << "-a", CB(OptionFoundTrue));

    // The next input should be treated as an argument.
    parser->parse(ARGS << "--aaa" << "foo", CB(Argument));
    parser->parse(ARGS << "-a" << "foo", CB(Argument));

    // The value should be booleanized.
    parser->parse(ARGS << "--aaa=foo", CB(OptionFoundTrue));
    parser->parse(ARGS << "--aaa=true", CB(OptionFoundTrue));
    parser->parse(ARGS << "--aaa=1", CB(OptionFoundTrue));
    parser->parse(ARGS << "--aaa=", CB(OptionFoundFalse));
    parser->parse(ARGS << "--aaa=False", CB(OptionFoundFalse));
    parser->parse(ARGS << "--aaa=0", CB(OptionFoundFalse));

    parser->parse(ARGS << "-a=foo", CB(OptionFoundTrue));
    parser->parse(ARGS << "-a=true", CB(OptionFoundTrue));
    parser->parse(ARGS << "-a=1", CB(OptionFoundTrue));
    parser->parse(ARGS << "-a=", CB(OptionFoundFalse));
    parser->parse(ARGS << "-a=False", CB(OptionFoundFalse));
    parser->parse(ARGS << "-a=0", CB(OptionFoundFalse));

    // Invalid forms.
    parser->parse(ARGS << "-aaa", CB(OptionUnknown));
    parser->parse(ARGS << "-aaa=foo", CB(OptionUnknown));
    parser->parse(ARGS << "--a", CB(OptionUnknown));
    parser->parse(ARGS << "--a=foo", CB(OptionUnknown));
}
