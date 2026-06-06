#include "mainwindow.h"

#include "../shared/fileutils.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static const qint64 kChunkSize = 64 * 1024;
static const int kFixedHeaderSize = int(sizeof(quint32) + sizeof(quint16) + sizeof(quint32));

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_outgoingSocket(new QTcpSocket(this)),
      m_server(new QTcpServer(this))
{
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    m_outgoingSocket->setProxy(QNetworkProxy::NoProxy);
    m_server->setProxy(QNetworkProxy::NoProxy);

    buildUi();

    connect(m_outgoingSocket, &QTcpSocket::connected, this, &MainWindow::onOutgoingConnected);
    connect(m_outgoingSocket, &QTcpSocket::disconnected, this, &MainWindow::onOutgoingDisconnected);
    connect(m_outgoingSocket, &QTcpSocket::bytesWritten, this, &MainWindow::onOutgoingBytesWritten);
    connect(m_outgoingSocket, &QTcpSocket::readyRead, this, &MainWindow::onOutgoingReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_outgoingSocket, &QTcpSocket::errorOccurred, this, &MainWindow::onOutgoingSocketError);
#else
    connect(m_outgoingSocket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this,
            &MainWindow::onOutgoingSocketError);
#endif
    connect(m_server, &QTcpServer::newConnection, this, &MainWindow::handleNewConnection);
}

MainWindow::~MainWindow()
{
    if (m_outgoingSocket->isOpen()) {
        m_outgoingSocket->disconnectFromHost();
    }
    if (m_incomingSocket) {
        m_incomingSocket->disconnectFromHost();
    }
    if (m_sendFile.isOpen()) {
        m_sendFile.close();
    }
    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    auto *sendGroup = new QGroupBox("Send Area", this);
    auto *sendLayout = new QVBoxLayout(sendGroup);
    auto *sendFormLayout = new QFormLayout;
    auto *sendFileLayout = new QHBoxLayout;
    auto *sendButtonLayout = new QHBoxLayout;

    m_peerIpEdit = new QLineEdit("127.0.0.1", this);
    m_peerPortEdit = new QLineEdit("8899", this);
    m_sendFileEdit = new QLineEdit(this);
    m_sendFileEdit->setReadOnly(true);
    m_browseSendFileButton = new QPushButton("Browse File", this);
    m_connectButton = new QPushButton("Connect Peer", this);
    m_disconnectButton = new QPushButton("Disconnect Peer", this);
    m_sendButton = new QPushButton("Send File", this);
    m_sendStatusLabel = new QLabel("Disconnected", this);
    m_sendProgressBar = new QProgressBar(this);
    m_sendProgressBar->setRange(0, 100);

    sendFileLayout->addWidget(m_sendFileEdit);
    sendFileLayout->addWidget(m_browseSendFileButton);

    sendButtonLayout->addWidget(m_connectButton);
    sendButtonLayout->addWidget(m_disconnectButton);
    sendButtonLayout->addWidget(m_sendButton);

    sendFormLayout->addRow("Peer IP:", m_peerIpEdit);
    sendFormLayout->addRow("Peer Port:", m_peerPortEdit);
    sendFormLayout->addRow("File Path:", sendFileLayout);
    sendFormLayout->addRow("Send Status:", m_sendStatusLabel);
    sendFormLayout->addRow("Send Progress:", m_sendProgressBar);

    sendLayout->addLayout(sendFormLayout);
    sendLayout->addLayout(sendButtonLayout);

    auto *receiveGroup = new QGroupBox("Receive Area", this);
    auto *receiveLayout = new QVBoxLayout(receiveGroup);
    auto *receiveFormLayout = new QFormLayout;
    auto *saveDirLayout = new QHBoxLayout;
    auto *listenButtonLayout = new QHBoxLayout;

    m_listenPortEdit = new QLineEdit("8899", this);
    m_saveDirEdit = new QLineEdit(QDir::currentPath(), this);
    m_browseSaveDirButton = new QPushButton("Browse Folder", this);
    m_startListenButton = new QPushButton("Start Listen", this);
    m_stopListenButton = new QPushButton("Stop Listen", this);
    m_receiveStatusLabel = new QLabel("Idle", this);
    m_receiveProgressBar = new QProgressBar(this);
    m_receiveProgressBar->setRange(0, 100);

    saveDirLayout->addWidget(m_saveDirEdit);
    saveDirLayout->addWidget(m_browseSaveDirButton);

    listenButtonLayout->addWidget(m_startListenButton);
    listenButtonLayout->addWidget(m_stopListenButton);

    receiveFormLayout->addRow("Listen Port:", m_listenPortEdit);
    receiveFormLayout->addRow("Save Folder:", saveDirLayout);
    receiveFormLayout->addRow("Receive Status:", m_receiveStatusLabel);
    receiveFormLayout->addRow("Receive Progress:", m_receiveProgressBar);

    receiveLayout->addLayout(receiveFormLayout);
    receiveLayout->addLayout(listenButtonLayout);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);

    mainLayout->addWidget(sendGroup);
    mainLayout->addWidget(receiveGroup);
    mainLayout->addWidget(m_logEdit);

    setCentralWidget(central);
    setWindowTitle("File Transfer Host");
    resize(860, 720);

    connect(m_browseSendFileButton, &QPushButton::clicked, this, &MainWindow::chooseSendFile);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::connectToPeer);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectFromPeer);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendFile);
    connect(m_browseSaveDirButton, &QPushButton::clicked, this, &MainWindow::chooseSaveDir);
    connect(m_startListenButton, &QPushButton::clicked, this, &MainWindow::startListening);
    connect(m_stopListenButton, &QPushButton::clicked, this, &MainWindow::stopListening);

    updateSendButtons();
    updateListenButtons();
}

void MainWindow::appendLog(const QString &message)
{
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(stamp, message));
}

void MainWindow::chooseSendFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "Select a file to send");
    if (filePath.isEmpty()) {
        return;
    }

    m_sendFileEdit->setText(filePath);
    appendLog(QString("Selected send file: %1").arg(filePath));
}

void MainWindow::chooseSaveDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "Select save folder", m_saveDirEdit->text());
    if (dir.isEmpty()) {
        return;
    }

    m_saveDirEdit->setText(dir);
    appendLog(QString("Receive folder changed to: %1").arg(dir));
}

void MainWindow::connectToPeer()
{
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    m_outgoingSocket->setProxy(QNetworkProxy::NoProxy);
    m_outgoingSocket->abort();
    m_sendStatusLabel->setText("Connecting...");
    appendLog(QString("Connecting to peer %1:%2").arg(m_peerIpEdit->text(), m_peerPortEdit->text()));
    m_outgoingSocket->connectToHost(m_peerIpEdit->text().trimmed(), m_peerPortEdit->text().toUShort());
}

void MainWindow::disconnectFromPeer()
{
    appendLog("Disconnect outgoing connection requested");
    m_outgoingSocket->disconnectFromHost();
}

bool MainWindow::prepareSelectedFile()
{
    const QString filePath = m_sendFileEdit->text().trimmed();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Notice", "Please select a file first.");
        return false;
    }

    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, "Notice", "The selected path is not a valid file.");
        return false;
    }

    m_sendMeta.fileName = info.fileName();
    m_sendMeta.fileSize = info.size();
    m_sendFile.setFileName(filePath);
    if (!m_sendFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open the selected file.");
        return false;
    }

    return true;
}

QByteArray MainWindow::buildHeader(const Protocol::FileMeta &meta) const
{
    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(QDataStream::Qt_5_12);
    payloadStream << meta.fileName << meta.fileSize;

    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);
    headerStream.setVersion(QDataStream::Qt_5_12);
    headerStream << Protocol::kMagic << Protocol::kVersion << quint32(payload.size());
    header.append(payload);
    return header;
}

void MainWindow::sendFile()
{
    if (m_outgoingSocket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "Notice", "Please connect to the peer first.");
        return;
    }

    resetSendState();
    if (!prepareSelectedFile()) {
        return;
    }

    const QByteArray header = buildHeader(m_sendMeta);
    m_totalBytesToSend = header.size() + m_sendMeta.fileSize;
    m_bytesScheduled += m_outgoingSocket->write(header);
    m_headerSent = true;
    m_waitingForAck = false;
    m_ackBuffer.clear();

    while (m_bytesScheduled < m_totalBytesToSend && m_outgoingSocket->bytesToWrite() < 2 * kChunkSize) {
        const QByteArray chunk = m_sendFile.read(kChunkSize);
        if (chunk.isEmpty()) {
            break;
        }
        m_bytesScheduled += m_outgoingSocket->write(chunk);
    }

    appendLog(QString("Start sending %1 (%2 bytes)").arg(m_sendMeta.fileName).arg(m_sendMeta.fileSize));
    m_sendStatusLabel->setText("Sending");
    updateSendButtons();
}

void MainWindow::startListening()
{
    const quint16 port = m_listenPortEdit->text().toUShort();
    if (!m_server->listen(QHostAddress::Any, port)) {
        QMessageBox::critical(this, "Error", m_server->errorString());
        appendLog(QString("Failed to listen: %1").arg(m_server->errorString()));
        return;
    }

    m_receiveStatusLabel->setText("Listening");
    appendLog(QString("Host listen started on port %1").arg(port));
    updateListenButtons();
}

void MainWindow::stopListening()
{
    if (m_incomingSocket) {
        m_incomingSocket->disconnectFromHost();
    }
    m_server->close();
    if (!m_incomingSocket) {
        m_receiveStatusLabel->setText("Idle");
    }
    appendLog("Host listen stopped");
    updateListenButtons();
}

void MainWindow::onOutgoingConnected()
{
    m_sendStatusLabel->setText("Connected");
    appendLog("Outgoing connection established");
    updateSendButtons();
}

void MainWindow::onOutgoingDisconnected()
{
    m_sendStatusLabel->setText("Disconnected");
    appendLog("Outgoing connection closed");
    resetSendState();
    updateSendButtons();
}

void MainWindow::onOutgoingBytesWritten(qint64)
{
    if (!m_headerSent) {
        return;
    }

    while (m_bytesScheduled < m_totalBytesToSend && m_outgoingSocket->bytesToWrite() < 2 * kChunkSize) {
        const QByteArray chunk = m_sendFile.read(kChunkSize);
        if (chunk.isEmpty()) {
            break;
        }
        m_bytesScheduled += m_outgoingSocket->write(chunk);
    }

    const qint64 sent = m_totalBytesToSend - m_outgoingSocket->bytesToWrite();
    const int progress = (m_totalBytesToSend > 0)
            ? int((sent * 100) / m_totalBytesToSend)
            : 0;
    m_sendProgressBar->setValue(progress);

    if (sent >= m_totalBytesToSend) {
        appendLog("All file bytes sent, waiting for receiver confirmation");
        m_sendProgressBar->setValue(100);
        m_waitingForAck = true;
        m_sendStatusLabel->setText("Waiting Ack");
        updateSendButtons();
    }
}

void MainWindow::onOutgoingReadyRead()
{
    m_ackBuffer.append(m_outgoingSocket->readAll());

    if (m_ackBuffer.contains(Protocol::kAckOk)) {
        finalizeSendSuccess();
        m_ackBuffer.clear();
        return;
    }

    if (m_ackBuffer.contains(Protocol::kAckFail)) {
        appendLog("Receiver reported a save failure");
        QMessageBox::warning(this, "Transfer Failed", "The receiver reported a save failure.");
        resetSendState();
        m_sendStatusLabel->setText("Ack Failed");
        updateSendButtons();
        m_ackBuffer.clear();
    }
}

void MainWindow::onOutgoingSocketError(QAbstractSocket::SocketError)
{
    appendLog(QString("Outgoing network error: %1").arg(m_outgoingSocket->errorString()));
    QMessageBox::warning(this, "Network Error", m_outgoingSocket->errorString());
    resetSendState();
    updateSendButtons();
}

void MainWindow::handleNewConnection()
{
    if (m_incomingSocket) {
        auto *extraClient = m_server->nextPendingConnection();
        extraClient->disconnectFromHost();
        extraClient->deleteLater();
        appendLog("Incoming connection rejected because another sender is active");
        return;
    }

    m_incomingSocket = m_server->nextPendingConnection();
    m_incomingSocket->setProxy(QNetworkProxy::NoProxy);
    connect(m_incomingSocket, &QTcpSocket::readyRead, this, &MainWindow::readIncomingData);
    connect(m_incomingSocket, &QTcpSocket::disconnected, this, &MainWindow::incomingDisconnected);

    resetReceiveState();
    m_receiveStatusLabel->setText("Connected");
    appendLog(QString("Incoming connection accepted from %1").arg(m_incomingSocket->peerAddress().toString()));
}

bool MainWindow::openOutputFile()
{
    QDir dir(m_saveDirEdit->text());
    if (!dir.exists() && !dir.mkpath(".")) {
        appendLog("Save folder does not exist and could not be created");
        return false;
    }

    const QString safeName = FileUtils::sanitizeFileName(m_receiveMeta.fileName);
    const QString filePath = buildAvailableReceivePath(safeName);

    m_outputFile.setFileName(filePath);
    if (!m_outputFile.open(QIODevice::WriteOnly)) {
        appendLog(QString("Failed to create output file: %1").arg(filePath));
        return false;
    }

    m_lastSavedFilePath = filePath;
    m_receiveProgressBar->setValue(0);
    appendLog(QString("Start receiving %1 into %2").arg(m_receiveMeta.fileName, filePath));
    return true;
}

QString MainWindow::buildAvailableReceivePath(const QString &fileName) const
{
    QDir dir(m_saveDirEdit->text());
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

void MainWindow::readIncomingData()
{
    if (!m_incomingSocket) {
        return;
    }

    m_receiveBuffer.append(m_incomingSocket->readAll());

    if (!m_headerReady) {
        if (!parseIncomingHeader()) {
            return;
        }
    }

    processIncomingChunks();
}

void MainWindow::incomingDisconnected()
{
    appendLog("Incoming connection closed");

    if (m_incomingSocket) {
        const QByteArray remainingBytes = m_incomingSocket->readAll();
        if (!remainingBytes.isEmpty()) {
            m_receiveBuffer.append(remainingBytes);
            if (!m_headerReady) {
                parseIncomingHeader();
            }
            processIncomingChunks();
        }
    }

    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }
    if (m_incomingSocket) {
        m_incomingSocket->deleteLater();
        m_incomingSocket = nullptr;
    }

    const bool receiveFinished = (m_receiveMeta.fileSize > 0 && m_receivedBytes >= m_receiveMeta.fileSize);
    if (!receiveFinished && m_receiveMeta.fileSize > 0) {
        appendLog(QString("Receive interrupted: got %1 / %2 bytes").arg(m_receivedBytes).arg(m_receiveMeta.fileSize));
    }
    resetReceiveState();
    m_receiveStatusLabel->setText(receiveFinished
                                  ? "Receive Done"
                                  : (m_server->isListening() ? "Listening" : "Idle"));
    updateListenButtons();
}

void MainWindow::resetSendState()
{
    if (m_sendFile.isOpen()) {
        m_sendFile.close();
    }

    m_totalBytesToSend = 0;
    m_bytesScheduled = 0;
    m_headerSent = false;
    m_waitingForAck = false;
    m_ackBuffer.clear();

    if (m_outgoingSocket->state() != QAbstractSocket::ConnectedState) {
        m_sendProgressBar->setValue(0);
    }
}

void MainWindow::resetReceiveState()
{
    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }

    m_receiveMeta = Protocol::FileMeta();
    m_headerReady = false;
    m_payloadSize = 0;
    m_receivedBytes = 0;
    m_receiveBuffer.clear();
    m_receiveProgressBar->setValue(0);
}

void MainWindow::prepareForNextIncomingFile()
{
    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }

    m_receiveMeta = Protocol::FileMeta();
    m_headerReady = false;
    m_payloadSize = 0;
    m_receivedBytes = 0;
}

bool MainWindow::parseIncomingHeader()
{
    if (m_receiveBuffer.size() < kFixedHeaderSize) {
        return false;
    }

    QDataStream fixedHeaderStream(m_receiveBuffer);
    fixedHeaderStream.setVersion(QDataStream::Qt_5_12);

    quint32 magic = 0;
    quint16 version = 0;
    quint32 payloadSize = 0;
    fixedHeaderStream >> magic >> version >> payloadSize;

    if (magic != Protocol::kMagic || version != Protocol::kVersion) {
        appendLog("Invalid protocol header on incoming connection");
        m_incomingSocket->disconnectFromHost();
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
        m_incomingSocket->write(Protocol::kAckFail);
        m_incomingSocket->flush();
        m_incomingSocket->disconnectFromHost();
        return false;
    }

    m_receiveStatusLabel->setText("Receiving");
    appendLog(QString("Receive header parsed, expecting %1 bytes").arg(m_receiveMeta.fileSize));
    return true;
}

void MainWindow::processIncomingChunks()
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
        m_receiveProgressBar->setValue(progress);

        if (m_receivedBytes >= m_receiveMeta.fileSize) {
            m_outputFile.flush();
            m_outputFile.close();
            m_receiveProgressBar->setValue(100);
            m_receiveStatusLabel->setText("Receive Done");
            appendLog(QString("Receive operation completed successfully, saved to %1").arg(m_lastSavedFilePath));
            if (m_incomingSocket) {
                m_incomingSocket->write(Protocol::kAckOk);
                m_incomingSocket->flush();
                m_incomingSocket->disconnectFromHost();
            }
            prepareForNextIncomingFile();
            break;
        }
    }
}

void MainWindow::finalizeSendSuccess()
{
    appendLog("Receiver confirmed that the file was saved successfully");
    m_sendStatusLabel->setText("Send Done");
    m_outgoingSocket->disconnectFromHost();
    resetSendState();
    updateSendButtons();
}

void MainWindow::updateSendButtons()
{
    const bool connected = (m_outgoingSocket->state() == QAbstractSocket::ConnectedState);
    const bool sending = m_headerSent || m_waitingForAck;

    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_sendButton->setEnabled(connected && !sending);
}

void MainWindow::updateListenButtons()
{
    const bool listening = m_server->isListening();
    m_startListenButton->setEnabled(!listening);
    m_stopListenButton->setEnabled(listening);
}
