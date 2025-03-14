#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTreeWidget>
#include <QLabel>

/**
 * @brief Класс Client представляет клиентское приложение для отображения данных с сервера
 * 
 * Класс отвечает за подключение к серверу, получение данных и их отображение
 * в древовидном виде.
 */
class Client : public QMainWindow
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса Client
     * @param parent Родительский виджет
     */
    explicit Client(QWidget *parent = nullptr);
    
    /**
     * @brief Деструктор класса Client
     */
    virtual ~Client();
    
    /**
     * @brief Получить адрес сервера
     * @return Строка с адресом сервера
     */
    QString getServerAddress() const;
    
    /**
     * @brief Установить адрес сервера
     * @param address Новый адрес сервера
     */
    void setServerAddress(const QString &address);
    
    /**
     * @brief Получить порт сервера
     * @return Номер порта сервера
     */
    int getServerPort() const;
    
    /**
     * @brief Установить порт сервера
     * @param port Новый номер порта
     */
    void setServerPort(int port);

public slots:
    /**
     * @brief Подключиться к серверу
     */
    void connectToServer();

private slots:
    void handleConnected();
    void handleDisconnected();
    void handleError(QAbstractSocket::SocketError error);
    void handleReadyRead();

private:
    /**
     * @brief Настройка пользовательского интерфейса
     */
    void setupUi();
    
    /**
     * @brief Обновление статуса подключения
     */
    void updateConnectionStatus();
    
    /**
     * @brief Обработка полученных данных JSON
     * @param jsonData Полученные данные в формате JSON
     */
    void processJsonData(const QByteArray &jsonData);

    // Сетевые компоненты
    QTcpSocket *socket;
    
    // UI компоненты
    QTreeWidget *treeWidget;
    QLabel *statusLabel;
    
    // Параметры подключения
    QString serverAddress;
    int serverPort;
    
    // Константы
    static const QString DEFAULT_SERVER_ADDRESS;
    static const int DEFAULT_SERVER_PORT = 12345;
    static const int RECONNECT_TIMEOUT_MS = 5000;
};

#endif // CLIENT_H 