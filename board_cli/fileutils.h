#ifndef BOARDCLI_FILEUTILS_H
#define BOARDCLI_FILEUTILS_H

#include <QString>

namespace BoardFileUtils {

QString sanitizeFileName(const QString &name);
QString buildAvailablePath(const QString &directory, const QString &fileName);

}

#endif // BOARDCLI_FILEUTILS_H
