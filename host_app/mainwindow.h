#ifndef HOSTAPP_MAINWINDOW_H
#define HOSTAPP_MAINWINDOW_H

#include "../shared/protocol.h"

#include <QFile>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QProgressBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void chooseSendFile();
    void chooseSaveDir();
    void connectToPeer();
    void disconnectFromPeer();
    void sendFile();
    void startListening();
    void stopListening();
    void onOutgoingConnected();
    void onOutgoingDisconnected();
    void onOutgoingBytesWritten(qint64 bytes);
    void onOutgoingReadyRead();
    void onOutgoingSocketError(QAbstractSocket::SocketError socketError);
    void handleNewConnection();
    void readIncomingData();
    void incomingDisconnected();

private:
    void buildUi();
    void appendLog(const QString &message);
    bool prepareSelectedFile();
    QByteArray buildHeader(const Protocol::FileMeta &meta) const;
    void resetSendState();
    void resetReceiveState();
    void prepareForNextIncomingFile();
    bool openOutputFile();
    QString buildAvailableReceivePath(const QString &fileName) const;
    bool parseIncomingHeader();
    void processIncomingChunks();
    void finalizeSendSuccess();
    void updateSendButtons();
    void updateListenButtons();

    QTcpSocket *m_outgoingSocket = nullptr;
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_incomingSocket = nullptr;

    QFile m_sendFile;
    QFile m_outputFile;
    Protocol::FileMeta m_sendMeta;
    Protocol::FileMeta m_receiveMeta;

    QLineEdit *m_peerIpEdit = nullptr;
    QLineEdit *m_peerPortEdit = nullptr;
    QLineEdit *m_sendFileEdit = nullptr;
    QPushButton *m_browseSendFileButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_sendStatusLabel = nullptr;
    QProgressBar *m_sendProgressBar = nullptr;

    QLineEdit *m_listenPortEdit = nullptr;
    QLineEdit *m_saveDirEdit = nullptr;
    QPushButton *m_browseSaveDirButton = nullptr;
    QPushButton *m_startListenButton = nullptr;
    QPushButton *m_stopListenButton = nullptr;
    QLabel *m_receiveStatusLabel = nullptr;
    QProgressBar *m_receiveProgressBar = nullptr;

    QTextEdit *m_logEdit = nullptr;

    qint64 m_totalBytesToSend = 0;
    qint64 m_bytesScheduled = 0;
    bool m_headerSent = false;
    bool m_waitingForAck = false;
    QByteArray m_ackBuffer;

    bool m_headerReady = false;
    quint32 m_payloadSize = 0;
    qint64 m_receivedBytes = 0;
    QByteArray m_receiveBuffer;
    QString m_lastSavedFilePath;
};

#endif // HOSTAPP_MAINWINDOW_H
