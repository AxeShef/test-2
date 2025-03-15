#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Настройка информации о приложении
    QApplication::setApplicationName("Qt Client");
    QApplication::setApplicationVersion("1.0");
    
    // Настройка парсера командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Клиент для получения данных с сервера");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Добавление опций командной строки
    QCommandLineOption consoleOption(QStringList() << "c" << "console", 
                                    "Запуск в консольном режиме без GUI");
    parser.addOption(consoleOption);
    
    QCommandLineOption addressOption(QStringList() << "a" << "address", 
                                    "Адрес сервера", "address", "localhost");
    parser.addOption(addressOption);
    
    QCommandLineOption portOption(QStringList() << "p" << "port", 
                                 "Порт сервера", "port", "12345");
    parser.addOption(portOption);
    
    // Обработка параметров командной строки
    parser.process(a);
    
    // Определение режима работы
    bool consoleMode = parser.isSet(consoleOption);
    
    // Создание клиента
    Client client(consoleMode);
    
    // Обработка параметров
    if (!client.parseCommandLineArgs(parser)) {
        return 1;
    }
    
    // Запуск в соответствующем режиме
    if (consoleMode) {
        return client.runConsoleMode();
    } else {
        client.connectToServer();
        client.show();
        return a.exec();
    }
} 