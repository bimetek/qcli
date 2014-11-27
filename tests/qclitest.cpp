#include "qclitest.h"

void QCliTest::init()
{
    parser = new CommandLineParser(this);
}

void QCliTest::cleanup()
{
    parser->deleteLater();
    parser = 0;
}
