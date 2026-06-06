#ifndef CLIENT_MAINWINDOW_H
#define CLIENT_MAINWINDOW_H

#include "../shared/protocol.h"

#include <QFile>
#include <QMainWindow>
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
    void chooseFile();
    void connectToServer();
    void disconnectFromServer();
    void sendFile();
    void onConnected();
    void onDisconnected();
    void onBytesWritten(qint64 bytes);
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    void buildUi();
    void appendLog(const QString &message);
    QByteArray buildHeader(const Protocol::FileMeta &meta) const;
    void resetSendState();
    bool prepareSelectedFile();

    QTcpSocket *m_socket = nullptr;
    QFile m_file;
    Protocol::FileMeta m_meta;

    QLineEdit *m_ipEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QLineEdit *m_fileEdit = nullptr;
    QPushButton *m_chooseButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QTextEdit *m_logEdit = nullptr;

    qint64 m_totalBytesToSend = 0;
    qint64 m_bytesScheduled = 0;
    bool m_headerSent = false;
};

#endif // CLIENT_MAINWINDOW_H
