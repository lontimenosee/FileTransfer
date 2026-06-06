#include "mainwindow.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static const qint64 kChunkSize = 64 * 1024;

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_socket(new QTcpSocket(this))
{
    buildUi();

    connect(m_socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &MainWindow::onBytesWritten);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);
}

MainWindow::~MainWindow()
{
    if (m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    auto *formLayout = new QFormLayout;
    auto *fileLayout = new QHBoxLayout;
    auto *buttonLayout = new QHBoxLayout;

    m_ipEdit = new QLineEdit("127.0.0.1", this);
    m_portEdit = new QLineEdit("8899", this);
    m_fileEdit = new QLineEdit(this);
    m_fileEdit->setReadOnly(true);

    m_chooseButton = new QPushButton("Browse File", this);
    m_connectButton = new QPushButton("Connect", this);
    m_disconnectButton = new QPushButton("Disconnect", this);
    m_sendButton = new QPushButton("Send File", this);
    m_statusLabel = new QLabel("Disconnected", this);
    m_progressBar = new QProgressBar(this);
    m_logEdit = new QTextEdit(this);

    m_progressBar->setRange(0, 100);
    m_logEdit->setReadOnly(true);
    m_disconnectButton->setEnabled(false);
    m_sendButton->setEnabled(false);

    fileLayout->addWidget(m_fileEdit);
    fileLayout->addWidget(m_chooseButton);

    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_sendButton);

    formLayout->addRow("Server IP:", m_ipEdit);
    formLayout->addRow("Server Port:", m_portEdit);
    formLayout->addRow("File Path:", fileLayout);
    formLayout->addRow("Connection:", m_statusLabel);
    formLayout->addRow("Progress:", m_progressBar);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_logEdit);

    setCentralWidget(central);
    setWindowTitle("File Transfer Client");
    resize(720, 480);

    connect(m_chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendFile);
}

void MainWindow::appendLog(const QString &message)
{
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(stamp, message));
}

void MainWindow::chooseFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "Select a file to send");
    if (filePath.isEmpty()) {
        return;
    }

    m_fileEdit->setText(filePath);
    appendLog(QString("Selected file: %1").arg(filePath));
}

void MainWindow::connectToServer()
{
    m_socket->abort();
    m_statusLabel->setText("Connecting...");
    appendLog(QString("Connecting to %1:%2").arg(m_ipEdit->text(), m_portEdit->text()));
    m_socket->connectToHost(m_ipEdit->text().trimmed(), m_portEdit->text().toUShort());
}

void MainWindow::disconnectFromServer()
{
    appendLog("Disconnect requested by user");
    m_socket->disconnectFromHost();
}

bool MainWindow::prepareSelectedFile()
{
    const QString filePath = m_fileEdit->text().trimmed();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Notice", "Please select a file first.");
        return false;
    }

    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, "Notice", "The selected path is not a valid file.");
        return false;
    }

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open the selected file.");
        return false;
    }

    m_meta.fileName = info.fileName();
    m_meta.fileSize = info.size();
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
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "Notice", "Please connect to the server first.");
        return;
    }

    if (!prepareSelectedFile()) {
        return;
    }

    resetSendState();

    const QByteArray header = buildHeader(m_meta);
    m_totalBytesToSend = header.size() + m_meta.fileSize;
    m_bytesScheduled += m_socket->write(header);
    m_headerSent = true;

    while (m_bytesScheduled < m_totalBytesToSend && m_socket->bytesToWrite() < 2 * kChunkSize) {
        const QByteArray chunk = m_file.read(kChunkSize);
        if (chunk.isEmpty()) {
            break;
        }
        m_bytesScheduled += m_socket->write(chunk);
    }

    appendLog(QString("Start sending %1 (%2 bytes)").arg(m_meta.fileName).arg(m_meta.fileSize));
    m_sendButton->setEnabled(false);
}

void MainWindow::onConnected()
{
    m_statusLabel->setText("Connected");
    m_connectButton->setEnabled(false);
    m_disconnectButton->setEnabled(true);
    m_sendButton->setEnabled(true);
    appendLog("Connected to server");
}

void MainWindow::onDisconnected()
{
    m_statusLabel->setText("Disconnected");
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
    m_sendButton->setEnabled(false);
    resetSendState();
    appendLog("Connection closed");
}

void MainWindow::onBytesWritten(qint64)
{
    if (!m_headerSent) {
        return;
    }

    while (m_bytesScheduled < m_totalBytesToSend && m_socket->bytesToWrite() < 2 * kChunkSize) {
        const QByteArray chunk = m_file.read(kChunkSize);
        if (chunk.isEmpty()) {
            break;
        }
        m_bytesScheduled += m_socket->write(chunk);
    }

    const qint64 sent = m_totalBytesToSend - m_socket->bytesToWrite();
    const int progress = (m_totalBytesToSend > 0)
            ? int((sent * 100) / m_totalBytesToSend)
            : 0;
    m_progressBar->setValue(progress);

    if (sent >= m_totalBytesToSend) {
        appendLog("File sent successfully");
        m_progressBar->setValue(100);
        resetSendState();
        m_sendButton->setEnabled(true);
    }
}

void MainWindow::onSocketError(QAbstractSocket::SocketError)
{
    appendLog(QString("Network error: %1").arg(m_socket->errorString()));
    QMessageBox::warning(this, "Network Error", m_socket->errorString());
    resetSendState();
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_sendButton->setEnabled(true);
    }
}

void MainWindow::resetSendState()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_totalBytesToSend = 0;
    m_bytesScheduled = 0;
    m_headerSent = false;
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        m_progressBar->setValue(0);
    }
}
