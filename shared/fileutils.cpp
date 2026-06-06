#include "fileutils.h"

#include <QRegularExpression>

namespace FileUtils {

QString sanitizeFileName(const QString &name)
{
    QString safeName = name.trimmed();
    if (safeName.isEmpty()) {
        safeName = "received_file.bin";
    }

    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    return safeName;
}

}
