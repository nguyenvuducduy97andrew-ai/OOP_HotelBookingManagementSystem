#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("data/hotel_data.db");
    
    if (!db.open()) {
        qDebug() << "Cannot open database:" << db.lastError().text();
        return 1;
    }
    
    QFile file("data/mock_data.sql");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open mock_data.sql";
        return 1;
    }
    
    QString sql = file.readAll();
    file.close();
    
    QStringList queries = sql.split(";`n");
    for (const QString& q : queries) {
        if (q.trimmed().isEmpty()) continue;
        QSqlQuery query(db);
        if (!query.exec(q)) {
            qDebug() << "Error executing query:" << query.lastError().text();
        }
    }
    
    qDebug() << "Mock data injected successfully!";
    db.close();
    return 0;
}
