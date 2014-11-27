#ifndef QCLI_GLOBAL_H
#define QCLI_GLOBAL_H

#include <QtCore/QtGlobal>

#if defined(QCLI_LIBRARY)
#  define QCLIISHARED_EXPORT Q_DECL_EXPORT
#else
#  define QCLIISHARED_EXPORT Q_DECL_IMPORT
#endif

#include <QDebug>

#endif // QCLI_GLOBAL_H
