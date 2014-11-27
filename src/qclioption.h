#ifndef QCLIOPTION_H
#define QCLIOPTION_H

#include "qcli_global.h"

namespace QCli
{

static QString OptionNamePrefix = "--";
static QString OptionAliasPrefix = "-";

enum OptionFlag
{
    OptionSwitch         = 0,
    OptionNegativeSwitch = 1,
    OptionValueRequired  = 1 << 1,
    OptionValueOptional  = 2 << 1,

    OptionValueNone      = OptionSwitch | OptionNegativeSwitch,
};

Q_FLAGS(OptionFlags)

Q_DECLARE_FLAGS(OptionFlags, OptionFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(OptionFlags)

}   // namespace QCli

#endif // QCLIOPTION_H

