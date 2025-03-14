#include "server.h"
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCoreApplication>

Server::Server(QObject *parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    
    // Инициализация базы данных
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("equipment.db");
    initDatabase();
}

bool Server::start(int port)
{
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Сервер не может запуститься. Ошибка:" << tcpServer->errorString();
        return false;
    }

    connect(tcpServer, &QTcpServer::newConnection, this, &Server::handleNewConnection);
    
    // Используем абсолютный путь
    QString execPath = QCoreApplication::applicationDirPath();
    QString equipmentPath = execPath + "/equipment";
    qDebug() << "Путь к папке equipment:" << equipmentPath;
    
    // Проверяем существование папки
    QDir dir(equipmentPath);
    if (!dir.exists()) {
        qDebug() << "Папка equipment не найдена!";
        return false;
    }
    
    // Чтение XML файлов из каталога
    parseXmlFiles(equipmentPath);
    
    qDebug() << "Сервер запущен на порту" << port;
    return true;
}

void Server::initDatabase()
{
    if (!db.open()) {
        qDebug() << "Ошибка открытия базы данных:" << db.lastError().text();
        return;
    }

    QSqlQuery query;
    // Создаем таблицу для оборудования
    query.exec("CREATE TABLE IF NOT EXISTS equipment ("
               "ip TEXT PRIMARY KEY,"
               "name TEXT,"
               "description TEXT"
               ")");
}

void Server::handleNewConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::handleReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::handleDisconnected);
    
    qDebug() << "Новое подключение от:" << clientSocket->peerAddress().toString();
}

void Server::handleReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray request = clientSocket->readAll();
    qDebug() << "Получен запрос от клиента:" << request;
    
    // Отправляем данные клиенту
    QByteArray jsonData = equipmentToJson();
    qDebug() << "Отправляем клиенту данные:" << jsonData;
    clientSocket->write(jsonData);
}

void Server::handleDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        qDebug() << "Клиент отключился:" << clientSocket->peerAddress().toString();
        clientSocket->deleteLater();
    }
}

void Server::parseXmlFiles(const QString &directory)
{
    QDir dir(directory);
    QStringList filters;
    filters << "*.xml";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &fileInfo : files) {
        parseXmlFile(fileInfo.filePath());
    }
}

void Server::parseXmlFile(const QString &filePath)
{
    qDebug() << "Чтение XML файла:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Не удалось открыть файл:" << filePath;
        return;
    }

    QXmlStreamReader xml(&file);
    Equipment equipment;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "block") {
                // Чтение атрибутов блока
                auto attrs = xml.attributes();
                equipment.blockId = attrs.value("id").toString();
                equipment.name = attrs.value("Name").toString();
                equipment.ip = attrs.value("IP").toString();
                equipment.boardCount = attrs.value("BoardCount").toInt();
                equipment.mtR = attrs.value("MtR").toInt();
                equipment.mtC = attrs.value("MtC").toInt();
                equipment.description = attrs.value("Description").toString();
                equipment.label = attrs.value("Label").toString();
            }
            else if (xml.name() == "board") {
                Board board;
                auto attrs = xml.attributes();
                board.id = attrs.value("id").toString();
                board.num = attrs.value("Num").toInt();
                board.name = attrs.value("Name").toString();
                board.portCount = attrs.value("PortCount").toInt();
                board.intLinks = attrs.value("IntLinks").toString();
                board.algorithms = attrs.value("Algoritms").toString();
                
                // Читаем порты платы
                while (!(xml.tokenType() == QXmlStreamReader::EndElement && 
                        xml.name() == "board")) {
                    if (xml.tokenType() == QXmlStreamReader::StartElement && 
                        xml.name() == "port") {
                        Port port;
                        auto portAttrs = xml.attributes();
                        port.id = portAttrs.value("id").toString();
                        port.num = portAttrs.value("Num").toInt();
                        port.media = portAttrs.value("Media").toInt();
                        port.signal = portAttrs.value("Signal").toInt();
                        board.ports.append(port);
                    }
                    xml.readNext();
                }
                
                equipment.boards.append(board);
            }
        }
    }

    if (xml.hasError()) {
        qDebug() << "Ошибка парсинга XML:" << xml.errorString();
        return;
    }

    file.close();
    saveEquipmentToDb(equipment);
}

void Server::saveEquipmentToDb(const Equipment &equipment)
{
    qDebug() << "Сохранение в БД оборудования с IP:" << equipment.ip;
    QSqlQuery query;
    
    // Сохраняем основную информацию об оборудовании
    query.prepare("INSERT OR REPLACE INTO equipment (ip, name, description) "
                 "VALUES (:ip, :name, :description)");
    query.bindValue(":ip", equipment.ip);
    query.bindValue(":name", equipment.name);
    query.bindValue(":description", equipment.description);
    
    if (!query.exec()) {
        qDebug() << "Ошибка сохранения в БД:" << query.lastError().text();
    } else {
        qDebug() << "Успешно сохранено в БД";
    }
}

void Server::sendDataToClient(QTcpSocket *client)
{
    QByteArray jsonData = equipmentToJson();
    client->write(jsonData);
}

QByteArray Server::equipmentToJson() const
{
    QJsonArray equipmentArray;
    QSqlQuery query("SELECT * FROM equipment");
    
    while (query.next()) {
        QJsonObject equipmentObject;
        equipmentObject["ip"] = query.value("ip").toString();
        equipmentObject["name"] = query.value("name").toString();
        equipmentObject["description"] = query.value("description").toString();
        equipmentArray.append(equipmentObject);
    }
    
    QJsonDocument doc(equipmentArray);
    return doc.toJson();
} 