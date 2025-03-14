#include "client.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QTimer>

// Инициализация статических констант
const QString Client::DEFAULT_SERVER_ADDRESS = "localhost";
const int Client::DEFAULT_SERVER_PORT;  // Уже инициализирован в заголовочном файле
const int Client::RECONNECT_TIMEOUT_MS;  // Уже инициализирован в заголовочном файле

Client::Client(QWidget *parent) : QMainWindow(parent)
{
    serverAddress = DEFAULT_SERVER_ADDRESS;
    serverPort = DEFAULT_SERVER_PORT;
    
    socket = new QTcpSocket(this);
    
    connect(socket, &QTcpSocket::connected, this, &Client::handleConnected);
    connect(socket, &QTcpSocket::disconnected, this, &Client::handleDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::handleReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::handleError);

    setupUi();
    connectToServer();
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
    
    updateConnectionStatus();
    socket->connectToHost(serverAddress, serverPort);
}

void Client::handleConnected()
{
    updateConnectionStatus();
    qDebug() << "Отправка запроса GET_DATA";
    socket->write("GET_DATA");
}

void Client::handleDisconnected()
{
    updateConnectionStatus();
    
    // Попытка переподключения через заданный интервал
    QTimer::singleShot(RECONNECT_TIMEOUT_MS, this, &Client::connectToServer);
}

void Client::handleError(QAbstractSocket::SocketError error)
{
    QString errorStr = "Ошибка подключения: " + socket->errorString();
    statusLabel->setText(errorStr);
    QMessageBox::critical(this, "Ошибка", errorStr);
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
        qDebug() << "Получены пустые данные";
        return;
    }
    
    qDebug() << "Получены данные:" << jsonData;
    processJsonData(jsonData);
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