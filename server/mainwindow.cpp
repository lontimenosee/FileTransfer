#include "mainwindow.h"

#include "../shared/fileutils.h"

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_server(new QTcpServer(this))
{
    buildUi();
    connect(m_server, &QTcpServer::newConnection, this, &MainWindow::handleNewConnection);
}

MainWindow::~MainWindow()
{
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
    }
    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    auto *formLayout = new QFormLayout;
    auto *dirLayout = new QHBoxLayout;
    auto *buttonLayout = new QHBoxLayout;

    m_portEdit = new QLineEdit("8899", this);
    m_saveDirEdit = new QLineEdit(QDir::currentPath(), this);
    m_chooseDirButton = new QPushButton("Browse Folder", this);
    m_startButton = new QPushButton("Start Listen", this);
    m_stopButton = new QPushButton("Stop Listen", this);
    m_statusLabel = new QLabel("Idle", this);
    m_progressBar = new QProgressBar(this);
    m_logEdit = new QTextEdit(this);

    m_progressBar->setRange(0, 100);
    m_logEdit->setReadOnly(true);
    m_stopButton->setEnabled(false);

    dirLayout->addWidget(m_saveDirEdit);
    dirLayout->addWidget(m_chooseDirButton);

    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);

    formLayout->addRow("Listen Port:", m_portEdit);
    formLayout->addRow("Save Folder:", dirLayout);
    formLayout->addRow("Status:", m_statusLabel);
    formLayout->addRow("Progress:", m_progressBar);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_logEdit);

    setCentralWidget(central);
    setWindowTitle("File Transfer Server");
    resize(720, 480);

    connect(m_chooseDirButton, &QPushButton::clicked, this, &MainWindow::chooseSaveDir);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startListening);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopListening);
}

void MainWindow::appendLog(const QString &message)
{
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(stamp, message));
}

void MainWindow::chooseSaveDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "Select save folder", m_saveDirEdit->text());
    if (dir.isEmpty()) {
        return;
    }
    m_saveDirEdit->setText(dir);
    appendLog(QString("Save folder changed to: %1").arg(dir));
}

void MainWindow::startListening()
{
    const quint16 port = m_portEdit->text().toUShort();
    if (!m_server->listen(QHostAddress::Any, port)) {
        QMessageBox::critical(this, "Error", m_server->errorString());
        appendLog(QString("Failed to listen: %1").arg(m_server->errorString()));
        return;
    }

    m_statusLabel->setText("Listening");
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    appendLog(QString("Server is listening on port %1").arg(port));
}

void MainWindow::stopListening()
{
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
    }
    m_server->close();
    m_statusLabel->setText("Idle");
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    appendLog("Server stopped listening");
}

void MainWindow::handleNewConnection()
{
    if (m_clientSocket) {
        auto *extraClient = m_server->nextPendingConnection();
        extraClient->disconnectFromHost();
        extraClient->deleteLater();
        appendLog("A client is already connected, new connection rejected");
        return;
    }

    m_clientSocket = m_server->nextPendingConnection();
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readClientData);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &MainWindow::clientDisconnected);

    appendLog(QString("Client connected: %1").arg(m_clientSocket->peerAddress().toString()));
    resetReceiveState();
}

bool MainWindow::openOutputFile()
{
    QDir dir(m_saveDirEdit->text());
    if (!dir.exists() && !dir.mkpath(".")) {
        appendLog("Save folder does not exist and could not be created");
        return false;
    }

    const QString safeName = FileUtils::sanitizeFileName(m_meta.fileName);
    const QString filePath = dir.filePath(safeName);

    m_outputFile.setFileName(filePath);
    if (!m_outputFile.open(QIODevice::WriteOnly)) {
        appendLog(QString("Failed to create output file: %1").arg(filePath));
        return false;
    }

    appendLog(QString("Start receiving %1, saving to %2").arg(m_meta.fileName, filePath));
    return true;
}

void MainWindow::readClientData()
{
    if (!m_clientSocket) {
        return;
    }

    if (!m_headerReady) {
        QDataStream stream(m_clientSocket);
        stream.setVersion(QDataStream::Qt_5_12);

        if (m_clientSocket->bytesAvailable() < static_cast<qint64>(sizeof(quint32) + sizeof(quint16) + sizeof(quint32))) {
            return;
        }

        quint32 magic = 0;
        quint16 version = 0;
        stream >> magic >> version >> m_payloadSize;

        if (magic != Protocol::kMagic || version != Protocol::kVersion) {
            appendLog("Invalid protocol header, closing connection");
            m_clientSocket->disconnectFromHost();
            return;
        }

        while (m_clientSocket->bytesAvailable() < m_payloadSize) {
            if (!m_clientSocket->waitForReadyRead(3000)) {
                return;
            }
        }

        QByteArray metaBytes = m_clientSocket->read(m_payloadSize);
        QDataStream metaStream(&metaBytes, QIODevice::ReadOnly);
        metaStream.setVersion(QDataStream::Qt_5_12);
        metaStream >> m_meta.fileName >> m_meta.fileSize;

        m_headerReady = true;
        if (!openOutputFile()) {
            m_clientSocket->disconnectFromHost();
            return;
        }
    }

    if (!m_outputFile.isOpen()) {
        return;
    }

    const QByteArray chunk = m_clientSocket->readAll();
    if (!chunk.isEmpty()) {
        m_outputFile.write(chunk);
        m_receivedBytes += chunk.size();

        const int progress = (m_meta.fileSize > 0)
                ? int((m_receivedBytes * 100) / m_meta.fileSize)
                : 0;
        m_progressBar->setValue(progress);

        if (m_receivedBytes >= m_meta.fileSize) {
            m_outputFile.flush();
            m_outputFile.close();
            m_progressBar->setValue(100);
            appendLog("File received successfully");
        }
    }
}

void MainWindow::clientDisconnected()
{
    appendLog("Client disconnected");

    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }

    if (m_clientSocket) {
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    resetReceiveState();
}

void MainWindow::resetReceiveState()
{
    if (m_outputFile.isOpen()) {
        m_outputFile.close();
    }

    m_meta = Protocol::FileMeta();
    m_headerReady = false;
    m_payloadSize = 0;
    m_receivedBytes = 0;
    m_progressBar->setValue(0);
}
