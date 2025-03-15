#include "client.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <QCoreApplication>

// Инициализация статических констант
const QString Client::DEFAULT_SERVER_ADDRESS = "localhost";
const int Client::DEFAULT_SERVER_PORT;  // Уже инициализирован в заголовочном файле
const int Client::RECONNECT_TIMEOUT_MS;  // Уже инициализирован в заголовочном файле
const int Client::CONSOLE_TIMEOUT_MS;  // Уже инициализирован в заголовочном файле

Client::Client(bool consoleMode, QWidget *parent) : QMainWindow(parent), 
    consoleOut(stdout), isConsoleMode(consoleMode), dataReceived(false)
{
    serverAddress = DEFAULT_SERVER_ADDRESS;
    serverPort = DEFAULT_SERVER_PORT;
    
    socket = new QTcpSocket(this);
    
    connect(socket, &QTcpSocket::connected, this, &Client::handleConnected);
    connect(socket, &QTcpSocket::disconnected, this, &Client::handleDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::handleReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::handleError);

    if (!isConsoleMode) {
        setupUi();
    }
}

Client::~Client()
{
    // Ресурсы, принадлежащие QObject, будут освобождены автоматически
    // благодаря системе родительских объектов Qt
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
}

QString Client::getServerAddress() const
{
    return serverAddress;
}

void Client::setServerAddress(const QString &address)
{
    if (serverAddress != address) {
        serverAddress = address;
        // Если сокет подключен, переподключаемся с новыми параметрами
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
            connectToServer();
        }
    }
}

int Client::getServerPort() const
{
    return serverPort;
}

void Client::setServerPort(int port)
{
    if (serverPort != port && port > 0 && port < 65536) {
        serverPort = port;
        // Если сокет подключен, переподключаемся с новыми параметрами
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
            connectToServer();
        }
    }
}

bool Client::parseCommandLineArgs(const QCommandLineParser &parser)
{
    if (parser.isSet("address")) {
        setServerAddress(parser.value("address"));
    }
    
    if (parser.isSet("port")) {
        bool ok;
        int port = parser.value("port").toInt(&ok);
        if (ok && port > 0 && port < 65536) {
            setServerPort(port);
        } else {
            if (isConsoleMode) {
                consoleOut << "Ошибка: Неверный порт. Должно быть число от 1 до 65535." << Qt::endl;
            } else {
                QMessageBox::warning(this, "Ошибка", "Неверный порт. Должно быть число от 1 до 65535.");
            }
            return false;
        }
    }
    
    return true;
}

int Client::runConsoleMode()
{
    consoleOut << "Запуск в консольном режиме" << Qt::endl;
    consoleOut << "Подключение к " << serverAddress << ":" << serverPort << "..." << Qt::endl;
    
    connectToServer();
    
    // Ожидаем получения данных или таймаута
    QTimer timer;
    QEventLoop loop;
    
    // Соединяем сигналы с выходом из цикла событий
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    connect(this, &Client::handleConsoleDataReceived, &loop, &QEventLoop::quit);
    
    timer.start(CONSOLE_TIMEOUT_MS);
    loop.exec();
    
    if (!dataReceived) {
        consoleOut << "Ошибка: Таймаут при ожидании данных от сервера." << Qt::endl;
        return 1;
    }
    
    return 0;
}

void Client::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabels({"IP", "Имя", "Описание"});
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setRootIsDecorated(false);
    treeWidget->setUniformRowHeights(true);
    treeWidget->setSortingEnabled(true);
    
    statusLabel = new QLabel("Статус подключения: Отключено", this);
    
    layout->addWidget(treeWidget);
    layout->addWidget(statusLabel);
    
    resize(800, 600);
    
    treeWidget->setColumnWidth(0, 150);
    treeWidget->setColumnWidth(1, 200);
    treeWidget->setColumnWidth(2, 300);
}

void Client::connectToServer()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        return; // Уже подключены
    }
    
    if (!isConsoleMode) {
        updateConnectionStatus();
    } else {
        consoleOut << "Подключение к серверу..." << Qt::endl;
    }
    
    socket->connectToHost(serverAddress, serverPort);
}

void Client::handleConnected()
{
    if (!isConsoleMode) {
        updateConnectionStatus();
    } else {
        consoleOut << "Подключено к серверу " << serverAddress << ":" << serverPort << Qt::endl;
        consoleOut << "Отправка запроса GET_DATA" << Qt::endl;
    }
    
    socket->write("GET_DATA");
}

void Client::handleDisconnected()
{
    if (!isConsoleMode) {
        updateConnectionStatus();
        // Попытка переподключения через заданный интервал
        QTimer::singleShot(RECONNECT_TIMEOUT_MS, this, &Client::connectToServer);
    } else {
        consoleOut << "Отключено от сервера" << Qt::endl;
    }
}

void Client::handleError(QAbstractSocket::SocketError error)
{
    QString errorStr = "Ошибка подключения: " + socket->errorString();
    
    if (!isConsoleMode) {
        statusLabel->setText(errorStr);
        QMessageBox::critical(this, "Ошибка", errorStr);
    } else {
        consoleOut << errorStr << Qt::endl;
    }
}

void Client::updateConnectionStatus()
{
    QString status = socket->state() == QAbstractSocket::ConnectedState ?
                    "Подключено" : "Отключено";
    statusLabel->setText("Статус подключения: " + status);
}

void Client::handleReadyRead()
{
    QByteArray jsonData = socket->readAll();
    
    if (jsonData.isEmpty()) {
        if (isConsoleMode) {
            consoleOut << "Получены пустые данные" << Qt::endl;
        } else {
            qDebug() << "Получены пустые данные";
        }
        return;
    }
    
    if (!isConsoleMode) {
        qDebug() << "Получены данные:" << jsonData;
        processJsonData(jsonData);
    } else {
        consoleOut << "Получены данные от сервера" << Qt::endl;
        printDataToConsole(jsonData);
        dataReceived = true;
        emit handleConsoleDataReceived();
    }
}

void Client::handleConsoleDataReceived()
{
    // Этот слот вызывается, когда данные получены в консольном режиме
    // Используется для выхода из цикла событий в runConsoleMode()
}

void Client::processJsonData(const QByteArray &jsonData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parsing error:" << parseError.errorString();
        return;
    }
    
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        treeWidget->clear();
        
        qDebug() << "Количество элементов в массиве:" << array.size();
        
        for (const QJsonValue &value : array) {
            QJsonObject obj = value.toObject();
            
            qDebug() << "Обработка объекта:";
            qDebug() << "IP:" << obj["ip"].toString();
            qDebug() << "Name:" << obj["name"].toString();
            qDebug() << "Description:" << obj["description"].toString();
            
            QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
            item->setText(0, obj["ip"].toString());
            item->setText(1, obj["name"].toString());
            item->setText(2, obj["description"].toString());
        }
        
        for (int i = 0; i < treeWidget->columnCount(); ++i) {
            treeWidget->resizeColumnToContents(i);
        }
    } else {
        qDebug() << "Полученные данные не являются массивом";
    }
}

void Client::printDataToConsole(const QByteArray &jsonData)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        consoleOut << "Ошибка разбора JSON: " << parseError.errorString() << Qt::endl;
        return;
    }
    
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        
        consoleOut << "Получено " << array.size() << " записей:" << Qt::endl;
        consoleOut << "----------------------------------------------" << Qt::endl;
        consoleOut << QString("%1 | %2 | %3").arg("IP", -15).arg("Имя", -20).arg("Описание") << Qt::endl;
        consoleOut << "----------------------------------------------" << Qt::endl;
        
        for (const QJsonValue &value : array) {
            QJsonObject obj = value.toObject();
            QString ip = obj["ip"].toString();
            QString name = obj["name"].toString();
            QString description = obj["description"].toString();
            
            consoleOut << QString("%1 | %2 | %3").arg(ip, -15).arg(name, -20).arg(description) << Qt::endl;
        }
        
        consoleOut << "----------------------------------------------" << Qt::endl;
    } else {
        consoleOut << "Полученные данные не являются массивом" << Qt::endl;
    }
} 