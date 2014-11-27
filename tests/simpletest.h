#ifndef SIMPLETEST_H
#define SIMPLETEST_H

#include "qclitest.h"

class SimpleTest : public QCliTest
{
    Q_OBJECT

private slots:
    void testRequired();
    void testOptional();
    void testSwitch();
};


#endif  // SIMPLETEST_H
