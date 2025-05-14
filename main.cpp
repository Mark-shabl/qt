
#include "databaseadmin.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Check if MySQL driver is available
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
                            QObject::tr("MySQL driver not available. Please install Qt MySQL plugin."));
        return 1;
    }

    DatabaseAdmin admin;
    admin.show();

    return app.exec();
}
