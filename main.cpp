// main.cpp
#include "databaseadmin.h"
#include <QApplication>
#include <QMessageBox>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);



    // Проверка доступности драйвера SQLite
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        QMessageBox::critical(nullptr, "Error", "SQLite driver not available");
        return 1;
    }

    DatabaseAdmin admin;
    admin.show();

    return a.exec();
}
