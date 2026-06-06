#ifndef BOARDCLI_RECEIVER_H
#define BOARDCLI_RECEIVER_H

#include "protocol.h"

#include <QFile>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class Receiver : public QObject
{
    Q_OBJECT

public:
    explicit Receiver(quint16 port, const QString &saveDirectory, QObject *parent = nullptr);

    bool start();

signals:
    void finished(int exitCode);

private slots:
    void handleNewConnection();
    void readClientData();
    void clientDisconnected();
    void serverError(QAbstractSocket::SocketError socketError);

private:
    void resetFileState();
    bool parseIncomingHeader();
    void processIncomingChunks();
    bool openOutputFile();
    void closeCurrentClient();
    void fail(const QString &message, int exitCode = 1);

    quint16 m_port = 0;
    QString m_saveDirectory;

    QTcpServer *m_server = nullptr;
    QTcpSocket *m_clientSocket = nullptr;

    QFile m_outputFile;
    BoardProtocol::FileMeta m_receiveMeta;
    QByteArray m_receiveBuffer;
    bool m_headerReady = false;
    quint32 m_payloadSize = 0;
    qint64 m_receivedBytes = 0;
    QString m_lastSavedFilePath;
};

#endif // BOARDCLI_RECEIVER_H
