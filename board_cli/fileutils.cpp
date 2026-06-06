#include "fileutils.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace BoardFileUtils {

QString sanitizeFileName(const QString &name)
{
    QString safeName = name.trimmed();
    if (safeName.isEmpty()) {
        safeName = "received_file.bin";
    }

    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    return safeName;
}

QString buildAvailablePath(const QString &directory, const QString &fileName)
{
    QDir dir(directory);
    QString candidate = dir.filePath(fileName);
    if (!QFileInfo::exists(candidate)) {
        return candidate;
    }

    QFileInfo info(fileName);
    const QString baseName = info.completeBaseName();
    const QString suffix = info.suffix();

    for (int index = 1; index < 10000; ++index) {
        const QString numberedName = suffix.isEmpty()
                ? QString("%1_%2").arg(baseName).arg(index)
                : QString("%1_%2.%3").arg(baseName).arg(index).arg(suffix);
        candidate = dir.filePath(numberedName);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return dir.filePath(QString("received_%1").arg(fileName));
}

}
