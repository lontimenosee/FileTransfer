#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QtGlobal>
#include <QByteArray>
#include <QString>

namespace Protocol {

static const quint32 kMagic = 0x46545231; // "FTR1"
static const quint16 kVersion = 1;
static const char kAckOk[] = "FILE_OK";
static const char kAckFail[] = "FILE_FAIL";

struct FileMeta
{
    QString fileName;
    qint64 fileSize = 0;
};

} // namespace Protocol

#endif // PROTOCOL_H
