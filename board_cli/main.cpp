#include "receiver.h"
#include "sender.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QStringList>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

QString expandUserPath(QString path)
{
    path = path.trimmed();
    if (path.isEmpty()) {
        return path;
    }

#ifndef Q_OS_WIN
    if (path == "~") {
        return QDir::homePath();
    }

    if (path.startsWith("~/")) {
        return QDir::homePath() + path.mid(1);
    }
#endif

    return path;
}

QString normalizeUserPath(const QString &path)
{
    return QDir::cleanPath(QDir::fromNativeSeparators(expandUserPath(path)));
}

void printErrorAndExit(const QString &message, int exitCode)
{
    QTextStream err(stderr);
    err << message << "\n";
    err.flush();
    QCoreApplication::exit(exitCode);
}

QString readConsoleLine()
{
#ifdef Q_OS_WIN
    HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    if (inputHandle != INVALID_HANDLE_VALUE && inputHandle != nullptr) {
        wchar_t buffer[2048];
        DWORD charsRead = 0;
        if (ReadConsoleW(inputHandle, buffer, DWORD(sizeof(buffer) / sizeof(wchar_t) - 1), &charsRead, nullptr) && charsRead > 0) {
            QString text = QString::fromWCharArray(buffer, int(charsRead));
            text.remove('\r');
            text.remove('\n');
            return text.trimmed();
        }
    }
#endif

    QTextStream in(stdin);
    return in.readLine().trimmed();
}

QString promptValue(const QString &label, const QString &defaultValue = QString())
{
    QTextStream out(stdout);

    if (defaultValue.isEmpty()) {
        out << label << ": ";
    } else {
        out << label << " [" << defaultValue << "]: ";
    }
    out.flush();

    const QString value = readConsoleLine();
    if (value.isEmpty()) {
        return defaultValue;
    }
    return value;
}

struct InteractiveOptions
{
    bool listenMode = false;
    bool sendMode = false;
    QString host;
    quint16 port = 8899;
    QString filePath;
    QString saveDir = ".";
};

QString promptExistingFilePath()
{
    while (true) {
        const QString filePath = normalizeUserPath(promptValue("File path to send"));
        if (filePath.isEmpty()) {
            QTextStream(stderr) << "File path cannot be empty. Enter q to quit.\n";
            continue;
        }
        if (filePath.compare("q", Qt::CaseInsensitive) == 0) {
            return QString();
        }

        QFileInfo info(filePath);
        if (info.exists() && info.isFile()) {
            return info.absoluteFilePath();
        }

        QTextStream(stderr) << "File does not exist. Please re-enter the path, or input q to quit.\n";
    }
}

QString promptDirectoryPath(const QString &defaultValue)
{
    while (true) {
        const QString dirPath = normalizeUserPath(promptValue("Save directory", defaultValue));
        if (dirPath.isEmpty()) {
            return defaultValue;
        }
        if (dirPath.compare("q", Qt::CaseInsensitive) == 0) {
            return QString();
        }
        return dirPath;
    }
}

quint16 promptPortValue(const QString &label, quint16 defaultPort)
{
    while (true) {
        const QString portText = promptValue(label, QString::number(defaultPort));
        if (portText.compare("q", Qt::CaseInsensitive) == 0) {
            return 0;
        }
        bool ok = false;
        const int port = portText.toInt(&ok);
        if (ok && port > 0 && port <= 65535) {
            return quint16(port);
        }
        QTextStream(stderr) << "Invalid port. Please enter a number between 1 and 65535, or q to quit.\n";
    }
}

bool collectInteractiveOptions(InteractiveOptions &options)
{
    QTextStream out(stdout);
    out << "board_cli interactive mode\n";
    out << "1. Receive file (listen mode)\n";
    out << "2. Send file\n";
    out.flush();

    const QString mode = promptValue("Choose mode", "1");
    if (mode == "1") {
        options.listenMode = true;
        const quint16 port = promptPortValue("Listen port", 8899);
        if (port == 0) {
            return false;
        }
        const QString saveDir = promptDirectoryPath(".");
        if (saveDir.isEmpty()) {
            return false;
        }
        options.port = port;
        options.saveDir = saveDir;
        return true;
    }

    if (mode == "2") {
        options.sendMode = true;
        const QString host = promptValue("Target host IP");
        if (host.isEmpty() || host.compare("q", Qt::CaseInsensitive) == 0) {
            QTextStream(stderr) << "Host cannot be empty.\n";
            return false;
        }
        const quint16 port = promptPortValue("Target port", 8899);
        if (port == 0) {
            return false;
        }
        const QString filePath = promptExistingFilePath();
        if (filePath.isEmpty()) {
            return false;
        }
        options.host = host;
        options.port = port;
        options.filePath = filePath;
        return true;
    }

    QTextStream(stderr) << "Unknown mode selection.\n";
    return false;
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("board_cli");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Headless file transfer tool for i.MX6U / Ubuntu");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption listenOption(QStringList() << "l" << "listen",
                                    "Run in receiver mode and listen for incoming files.");
    QCommandLineOption sendOption(QStringList() << "s" << "send",
                                  "Run in sender mode and send one file.");
    QCommandLineOption hostOption(QStringList() << "host",
                                  "Target host IP or hostname.",
                                  "host");
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "Target or listen port.",
                                  "port",
                                  "8899");
    QCommandLineOption fileOption(QStringList() << "f" << "file",
                                  "File path to send.",
                                  "file");
    QCommandLineOption saveDirOption(QStringList() << "d" << "save-dir",
                                     "Directory used to save received files.",
                                     "directory",
                                     ".");
    QCommandLineOption interactiveOption(QStringList() << "i" << "interactive",
                                         "Start in interactive prompt mode.");

    parser.addOption(listenOption);
    parser.addOption(sendOption);
    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(fileOption);
    parser.addOption(saveDirOption);
    parser.addOption(interactiveOption);
    parser.process(app);

    InteractiveOptions options;

    const bool noExplicitMode = !parser.isSet(listenOption) && !parser.isSet(sendOption);
    if (parser.isSet(interactiveOption) || noExplicitMode) {
        if (!collectInteractiveOptions(options)) {
            return 1;
        }
    } else {
        options.listenMode = parser.isSet(listenOption);
        options.sendMode = parser.isSet(sendOption);

        if (options.listenMode == options.sendMode) {
            printErrorAndExit("Please choose exactly one mode: --listen or --send", 1);
            return app.exec();
        }

        options.port = parser.value(portOption).toUShort();
        if (options.port == 0) {
            printErrorAndExit("Port must be a valid positive number.", 1);
            return app.exec();
        }

        options.host = parser.value(hostOption).trimmed();
        options.filePath = normalizeUserPath(parser.value(fileOption));
        options.saveDir = normalizeUserPath(parser.value(saveDirOption));
    }

    if (options.sendMode) {
        if (options.host.isEmpty()) {
            printErrorAndExit("Send mode requires --host", 1);
            return app.exec();
        }
        if (options.filePath.isEmpty()) {
            printErrorAndExit("Send mode requires --file", 1);
            return app.exec();
        }

        Sender sender(options.host, options.port, options.filePath);
        QObject::connect(&sender, &Sender::finished, &app, &QCoreApplication::exit);
        sender.start();
        return app.exec();
    }

    Receiver receiver(options.port, options.saveDir);
    QObject::connect(&receiver, &Receiver::finished, &app, &QCoreApplication::exit);
    if (!receiver.start()) {
        return 1;
    }

    return app.exec();
}
