#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    Server server;
    if (!server.start(12345)) {
        return -1;
    }
    
    return a.exec();
} 