#ifndef BOARDCLI_SENDER_H
#define BOARDCLI_SENDER_H

#include "protocol.h"

#include <QFile>
#include <QObject>
#include <QTcpSocket>

class Sender : public QObject
{
    Q_OBJECT

public:
    explicit Sender(const QString &host,
                    quint16 port,
                    const QString &filePath,
                    QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);

private slots:
    void onConnected();
    void onBytesWritten(qint64 bytes);
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    bool prepareFile();
    QByteArray buildHeader(const BoardProtocol::FileMeta &meta) const;
    void feedFileChunks();
    void fail(const QString &message, int exitCode = 1);
    void succeed();

    QString m_host;
    quint16 m_port = 0;
    QString m_filePath;

    QTcpSocket *m_socket = nullptr;
    QFile m_file;
    BoardProtocol::FileMeta m_meta;

    qint64 m_totalBytesToSend = 0;
    qint64 m_bytesScheduled = 0;
    bool m_headerSent = false;
    bool m_waitingForAck = false;
    QByteArray m_ackBuffer;
    bool m_finished = false;
};

#endif // BOARDCLI_SENDER_H
