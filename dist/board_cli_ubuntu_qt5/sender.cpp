#include "sender.h"

#include <QDataStream>
#include <QFileInfo>
#include <QTextStream>

namespace {

static const qint64 kChunkSize = 64 * 1024;

void logLine(const QString &text)
{
    QTextStream out(stdout);
    out << text << "\n";
    out.flush();
}

void logError(const QString &text)
{
    QTextStream err(stderr);
    err << text << "\n";
    err.flush();
}

}

Sender::Sender(const QString &host, quint16 port, const QString &filePath, QObject *parent)
    : QObject(parent),
      m_host(host),
      m_port(port),
      m_filePath(filePath),
      m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &Sender::onConnected);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &Sender::onBytesWritten);
    connect(m_socket, &QTcpSocket::readyRead, this, &Sender::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &Sender::onDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &Sender::onSocketError);
#else
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this,
            &Sender::onSocketError);
#endif
}

void Sender::start()
{
    if (!prepareFile()) {
        return;
    }

    logLine(QString("Connecting to %1:%2").arg(m_host).arg(m_port));
    m_socket->connectToHost(m_host, m_port);
}

bool Sender::prepareFile()
{
    QFileInfo info(m_filePath);
    if (!info.exists() || !info.isFile()) {
        fail(QString("Send file does not exist: %1").arg(m_filePath));
        return false;
    }

    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        fail(QString("Failed to open send file: %1").arg(m_filePath));
        return false;
    }

    m_meta.fileName = info.fileName();
    m_meta.fileSize = info.size();
    return true;
}

QByteArray Sender::buildHeader(const BoardProtocol::FileMeta &meta) const
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(QDataStream::Qt_5_12);
    payloadStream << meta.fileName << meta.fileSize;

    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);
    headerStream.setVersion(QDataStream::Qt_5_12);
    headerStream << BoardProtocol::kMagic << BoardProtocol::kVersion << quint32(payload.size());
    header.append(payload);
    return header;
}

void Sender::onConnected()
{
    const QByteArray header = buildHeader(m_meta);
    m_totalBytesToSend = header.size() + m_meta.fileSize;
    m_bytesScheduled = m_socket->write(header);
    m_headerSent = true;
    m_waitingForAck = false;
    m_ackBuffer.clear();

    logLine(QString("Start sending %1 (%2 bytes)").arg(m_meta.fileName).arg(m_meta.fileSize));
    feedFileChunks();
}

void Sender::feedFileChunks()
{
    while (m_bytesScheduled < m_totalBytesToSend && m_socket->bytesToWrite() < 2 * kChunkSize) {
        const QByteArray chunk = m_file.read(kChunkSize);
        if (chunk.isEmpty()) {
            break;
        }
        m_bytesScheduled += m_socket->write(chunk);
    }
}

void Sender::onBytesWritten(qint64)
{
    if (!m_headerSent || m_finished) {
        return;
    }

    feedFileChunks();

    const qint64 sent = m_totalBytesToSend - m_socket->bytesToWrite();
    const int progress = (m_totalBytesToSend > 0)
            ? int((sent * 100) / m_totalBytesToSend)
            : 0;
    logLine(QString("Send progress: %1%").arg(progress));

    if (sent >= m_totalBytesToSend && !m_waitingForAck) {
        m_waitingForAck = true;
        logLine("All file bytes sent, waiting for receiver ACK");
    }
}

void Sender::onReadyRead()
{
    if (m_finished) {
        return;
    }

    m_ackBuffer.append(m_socket->readAll());

    if (m_ackBuffer.contains(BoardProtocol::kAckOk)) {
        succeed();
        return;
    }

    if (m_ackBuffer.contains(BoardProtocol::kAckFail)) {
        fail("Receiver reported save failure.");
    }
}

void Sender::onDisconnected()
{
    if (m_finished) {
        return;
    }

    if (m_waitingForAck) {
        fail("Connection closed before ACK was received.");
    } else {
        fail("Connection closed unexpectedly.");
    }
}

void Sender::onSocketError(QAbstractSocket::SocketError)
{
    if (m_finished) {
        return;
    }

    fail(QString("Socket error: %1").arg(m_socket->errorString()));
}

void Sender::fail(const QString &message, int exitCode)
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    if (m_file.isOpen()) {
        m_file.close();
    }
    logError(message);
    emit finished(exitCode);
}

void Sender::succeed()
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    if (m_file.isOpen()) {
        m_file.close();
    }
    logLine("Receiver confirmed save success.");
    emit finished(0);
}
