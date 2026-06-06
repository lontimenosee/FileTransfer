#ifndef SERVER_MAINWINDOW_H
#define SERVER_MAINWINDOW_H

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
    void chooseSaveDir();
    void startListening();
    void stopListening();
    void handleNewConnection();
    void readClientData();
    void clientDisconnected();

private:
    void buildUi();
    void appendLog(const QString &message);
    void resetReceiveState();
    bool openOutputFile();

    QTcpServer *m_server = nullptr;
    QTcpSocket *m_clientSocket = nullptr;
    QFile m_outputFile;
    Protocol::FileMeta m_meta;

    QLineEdit *m_portEdit = nullptr;
    QLineEdit *m_saveDirEdit = nullptr;
    QPushButton *m_chooseDirButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QTextEdit *m_logEdit = nullptr;

    bool m_headerReady = false;
    quint32 m_payloadSize = 0;
    qint64 m_receivedBytes = 0;
};

#endif // SERVER_MAINWINDOW_H
