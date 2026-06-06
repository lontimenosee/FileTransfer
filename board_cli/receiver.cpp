#include "receiver.h"

#include "fileutils.h"

#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

namespace {

static const int kFixedHeaderSize = int(sizeof(quint32) + sizeof(quint16) + sizeof(quint32));

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

Receiver::Receiver(quint16 port, const QString &saveDirectory, QObject *parent)
    : QObject(parent),
      m_port(port),
      m_saveDirectory(saveDirectory),
      m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &Receiver::handleNewConnection);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_server, &QTcpServer::acceptError, this, &Receiver::serverError);
#endif
}

bool Receiver::start()
{
    QDir dir(m_saveDirectory);
    if (!dir.exists() && !dir.mkpath(".")) {
        fail(QString("Failed to create save directory: %1").arg(m_saveDirectory));
        return false;
    }

    if (!m_server->listen(QHostAddress::Any, m_port)) {
        fail(QString("Listen failed: %1").arg(m_server->errorString()));
        return false;
    }

    logLine(QString("board_cli listen mode started on port %1").arg(m_port));
    logLine(QString("Save directory: %1").arg(QDir(m_saveDirectory).absolutePath()));
    return true;
}

void Receiver::handleNewConnection()
{
    if (m_clientSocket) {
        QTcpSocket *extraClient = m_server->nextPendingConnection();
        extraClient->disconnectFromHost();
        extraClient->deleteLater();
        logLine("A client is already active, new connection rejected.");
        return;
    }

    m_clientSocket = m_server->nextPendingConnection();
    resetFileState();
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &Receiver::readClientData);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &Receiver::clientDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_clientSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (m_clientSocket) {
            logError(QString("Client socket error: %1").arg(m_clientSocket->errorString()));
        }
    });
#else
    connect(m_clientSocket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this,
            [this](QAbstractSocket::SocketError) {
                if (m_clientSocket) {
                    logError(QString("Client socket error: %1").arg(m_clientSocket->errorString()));
                }
            });
#endif
    logLine(QString("Client connected from %1").arg(m_clientSocket->peerAddress().toString()));
}

void Receiver::readClientData()
{
    if (!m_clientSocket) {
        return;
    }

    m_receiveBuffer.append(m_clientSocket->readAll());

    if (!m_headerReady) {
        if (!parseIncomingHeader()) {
            return;
        }
    }

    processIncomingChunks();
}

bool Receiver::parseIncomingHeader()
{
    if (m_receiveBuffer.size() < kFixedHeaderSize) {
        return false;
    }

    QDataStream headerStream(m_receiveBuffer);
    headerStream.setVersion(QDataStream::Qt_5_12);

    quint32 magic = 0;
    quint16 version = 0;
    quint32 payloadSize = 0;
    headerStream >> magic >> version >> payloadSize;

    if (magic != BoardProtocol::kMagic || version != BoardProtocol::kVersion) {
        if (m_clientSocket) {
            m_clientSocket->write(BoardProtocol::kAckFail);
            m_clientSocket->flush();
        }
        fail("Invalid protocol header received.");
        return false;
    }

    if (m_receiveBuffer.size() < kFixedHeaderSize + static_cast<int>(payloadSize)) {
        return false;
    }

    const QByteArray metaBytes = m_receiveBuffer.mid(kFixedHeaderSize, payloadSize);
    QDataStream metaStream(metaBytes);
    metaStream.setVersion(QDataStream::Qt_5_12);
    metaStream >> m_receiveMeta.fileName >> m_receiveMeta.fileSize;

    m_payloadSize = payloadSize;
    m_receiveBuffer.remove(0, kFixedHeaderSize + static_cast<int>(payloadSize));
    m_headerReady = true;

    if (!openOutputFile()) {
        if (m_clientSocket) {
            m_clientSocket->write(BoardProtocol::kAckFail);
            m_clientSocket->flush();
        }
        return false;
    }

    logLine(QString("Receive header parsed, expecting %1 bytes").arg(m_receiveMeta.fileSize));
    return true;
}

bool Receiver::openOutputFile()
{
    const QString safeName = BoardFileUtils::sanitizeFileName(m_receiveMeta.fileName);
    const QString filePath = BoardFileUtils::buildAvailablePath(m_saveDirectory, safeName);

    m_outputFile.setFileName(filePath);
    if (!m_outputFile.open(QIODevice::WriteOnly)) {
        logError(QString("Failed to open output file: %1").arg(filePath));
        return false;
    }

    m_lastSavedFilePath = filePath;
    logLine(QString("Start receiving %1 into %2").arg(m_receiveMeta.fileName).arg(filePath));
    return true;
}

void Receiver::processIncomingChunks()
{
    if (!m_headerReady || !m_outputFile.isOpen()) {
        return;
    }

    while (m_headerReady && m_outputFile.isOpen()) {
        const qint64 remaining = m_receiveMeta.fileSize - m_receivedBytes;
        if (remaining <= 0) {
            break;
        }

        const qint64 chunkBytes = qMin<qint64>(remaining, m_receiveBuffer.size());
        if (chunkBytes <= 0) {
            break;
        }

        const QByteArray chunk = m_receiveBuffer.left(int(chunkBytes));
        m_receiveBuffer.remove(0, int(chunkBytes));
        m_outputFile.write(chunk);
        m_receivedBytes += chunkBytes;

        const int progress = (m_receiveMeta.fileSize > 0)
                ? int((m_receivedBytes * 100) / m_receiveMeta.fileSize)
                : 0;
        logLine(QString("Receive progress: %1%").arg(progress));

        if (m_receivedBytes >= m_receiveMeta.fileSize) {
            m_outputFile.flush();
            m_outputFile.close();
            logLine(QString("Receive completed successfully, saved to %1").arg(m_lastSavedFilePath));
            if (m_clientSocket) {
                m_clientSocket->write(BoardProtocol::kAckOk);
                m_clientSocket->flush();
            }
            resetFileState();
            break;
        }
    }
}

void Receiver::clientDisconnected()
{
    if (m_clientSocket) {
        const QByteArray remaining = m_clientSocket->readAll();
        if (!remaining.isEmpty()) {
            m_receiveBuffer.append(remaining);
            if (!m_headerReady) {
                parseIncomingHeader();
            }
            processIncomingChunks();
        }
    }

    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }

    if (m_receiveMeta.fileSize > 0 && m_receivedBytes < m_receiveMeta.fileSize) {
        logError(QString("Transfer interrupted: received %1 / %2 bytes")
                 .arg(m_receivedBytes)
                 .arg(m_receiveMeta.fileSize));
    } else {
        logLine("Client disconnected.");
    }

    if (m_clientSocket) {
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    resetFileState();
}

void Receiver::serverError(QAbstractSocket::SocketError)
{
    fail(QString("Server error: %1").arg(m_server->errorString()));
}

void Receiver::resetFileState()
{
    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }

    m_receiveMeta = BoardProtocol::FileMeta();
    m_receiveBuffer.clear();
    m_headerReady = false;
    m_payloadSize = 0;
    m_receivedBytes = 0;
    m_lastSavedFilePath.clear();
}

void Receiver::closeCurrentClient()
{
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }
}

void Receiver::fail(const QString &message, int exitCode)
{
    logError(message);
    closeCurrentClient();
    emit finished(exitCode);
}
