#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QXmlStreamReader>

struct Port {
    QString id;
    int num;
    int media;
    int signal;
};

struct Board {
    QString id;
    int num;
    QString name;
    int portCount;
    QString intLinks;
    QString algorithms;
    QList<Port> ports;
};

struct Equipment {
    QString blockId;
    QString name;
    QString ip;
    int boardCount;
    int mtR;
    int mtC;
    QString description;
    QString label;
    QList<Board> boards;
};

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    bool start(int port = 12345);

private slots:
    void handleNewConnection();
    void handleReadyRead();
    void handleDisconnected();

private:
    void parseXmlFiles(const QString &directory);
    void initDatabase();
    void sendDataToClient(QTcpSocket *client);
    QList<Equipment> equipmentList;
    void parseXmlFile(const QString &filePath);
    void saveEquipmentToDb(const Equipment &equipment);
    QByteArray equipmentToJson() const;

    QTcpServer *tcpServer;
    QSqlDatabase db;
};

#endif // SERVER_H 