#include "DataManager.h"
#include "SuiteRoom.h"
#include "DeluxeRoom.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>

// Creates or returns the one shared DataManager instance.
//reference helps ensuring only one DataManager instance exist
DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

bool DataManager::initDatabase(const std::string& dataPath) {
    if (m_db.isOpen() && m_db.databaseName() == QString::fromStdString(dataPath)) {
        return true;
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(QString::fromStdString(dataPath));

    if (!m_db.open()) {
        qDebug() << "SQLite connection error: " << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;

    query.exec("PRAGMA foreign_keys = ON;");

    QString createCustomer =
        "CREATE TABLE IF NOT EXISTS Customer ("
        "   customerId TEXT PRIMARY KEY,"
        "   name TEXT NOT NULL,"
        "   phoneNumber TEXT"
        ");";
    if (!query.exec(createCustomer)) {
        qDebug() << "Error creating Customer table: " << query.lastError().text();
        return false;
    }

    QString createRoom =
        "CREATE TABLE IF NOT EXISTS Room ("
        "   roomNumber TEXT PRIMARY KEY,"
        "   basePrice REAL NOT NULL,"
        "   isAvailable INTEGER DEFAULT 1,"
        "   roomType TEXT NOT NULL,"
        "   premiumServiceFee REAL,"
        "   miniBarFee REAL"
        ");";
    if (!query.exec(createRoom)) {
        qDebug() << "Error creating Room table: " << query.lastError().text();
        return false;
    }

    QString createBooking =
        "CREATE TABLE IF NOT EXISTS Booking ("
        "   bookingId TEXT PRIMARY KEY,"
        "   customerId TEXT NOT NULL,"
        "   roomNumber TEXT NOT NULL,"
        "   checkInDate TEXT NOT NULL,"
        "   checkOutDate TEXT NOT NULL,"
        "   FOREIGN KEY (customerId) REFERENCES Customer(customerId) ON DELETE CASCADE ON UPDATE CASCADE,"
        "   FOREIGN KEY (roomNumber) REFERENCES Room(roomNumber) ON DELETE RESTRICT ON UPDATE CASCADE"
        ");";
    if (!query.exec(createBooking)) {
        qDebug() << "Error creating Booking table: " << query.lastError().text();
        return false;
    }

    QString createInvoice =
        "CREATE TABLE IF NOT EXISTS Invoice ("
        "   invoiceId TEXT PRIMARY KEY,"
        "   bookingId TEXT NOT NULL,"
        "   totalAmount REAL NOT NULL,"
        "   paymentDate TEXT NOT NULL,"
        "   FOREIGN KEY (bookingId) REFERENCES Booking(bookingId) ON DELETE CASCADE ON UPDATE CASCADE"
        ");";
    if (!query.exec(createInvoice)) {
        qDebug() << "Error creating Invoicce table: " << query.lastError().text();
        return false;
    }

    qDebug() << "The SQLite database is all set and ready!";
    return true;
}

// Loads application data from the configured storage location.
bool DataManager::loadAll(HotelManager& manager, const std::string& dataPath) {
    if (!initDatabase(dataPath)) {
        return false;
    }

    Booking::initCounterFromDatabase();

    QSqlQuery query;
    std::string errorMsg;

    // Tải thông tin khách hàng từ DB vào bộ nhớ quản lý
    if (query.exec("SELECT customerId, name, phoneNumber FROM Customer")) {
        while (query.next()) {
            std::string custId = query.value(0).toString().toStdString();
            std::string name = query.value(1).toString().toStdString();
            std::string phone = query.value(2).toString().toStdString();

            manager.registerCustomer(custId, name, phone, errorMsg);
        }
    } else {
        qDebug() << "Error reading Customer table: " << query.lastError().text();
        return false;
    }

    // Tải danh sách phòng và phục hồi thuộc tính riêng của từng loại phòng con
    if (query.exec("SELECT roomNumber, basePrice, isAvailable, roomType, premiumServiceFee, miniBarFee FROM Room")) {
        while (query.next()) {
            std::string roomNum = query.value(0).toString().toStdString();
            double price = query.value(1).toDouble();
            int isAvailable = query.value(2).toInt();
            std::string typeStr = query.value(3).toString().toStdString();
            double premiumFee = query.value(4).toDouble();
            double miniBarFee = query.value(5).toDouble();

            RoomType type = RoomType::Standard;
            if (typeStr == "Deluxe") type = RoomType::Deluxe;
            else if (typeStr == "Suite") type = RoomType::Suite;

            manager.registerRoom(type, roomNum, price, errorMsg);

            auto room = manager.findRoomByNumber(roomNum);
            if (room) {
                room->setIsAvailable(isAvailable == 1);

                // Dùng std::dynamic_pointer_cast vì dự án sử dụng std::shared_ptr
                if (type == RoomType::Suite) {
                    auto suite = std::dynamic_pointer_cast<SuiteRoom>(room);
                    if (suite) suite->setPremiumServiceFee(premiumFee);
                }
                else if (type == RoomType::Deluxe) {
                    auto deluxe = std::dynamic_pointer_cast<DeluxeRoom>(room);
                    if (deluxe) deluxe->setMiniBarFee(miniBarFee);
                }
            }
        }
    } else {
        qDebug() << "Error reading Room table: " << query.lastError().text();
        return false;
    }

    // Tải lịch đặt phòng, đổi từ QString sang std::string bằng .toStdString()
    if (query.exec("SELECT customerId, roomNumber, checkInDate, checkOutDate FROM Booking")) {
        while (query.next()) {
            std::string custId = query.value(0).toString().toStdString();
            std::string roomNum = query.value(1).toString().toStdString();

            std::string checkInStr = query.value(2).toString().toStdString();
            std::string checkOutStr = query.value(3).toString().toStdString();

            manager.createBooking(custId, roomNum, checkInStr, checkOutStr, errorMsg);
        }
    } else {
        qDebug() << "Error reading Booking table: " << query.lastError().text();
        return false;
    }

    return true;
}

// Saves application data to the configured storage location.
bool DataManager::saveAll(const HotelManager& manager, const std::string& dataPath) {
    if (!initDatabase(dataPath)) {
        return false;
    }

    QSqlQuery query;

    // 1. Làm sạch các bảng cũ trước khi ghi đè dữ liệu mới
    query.exec("DELETE FROM Booking");
    query.exec("DELETE FROM Room");
    query.exec("DELETE FROM Customer");

    // 2. Lưu danh sách Customer
    query.prepare("INSERT INTO Customer (customerId, name, phoneNumber) VALUES (?, ?, ?)");
    for (const auto& customer : manager.getCustomers()) {
        if (customer) {
            query.addBindValue(QString::fromStdString(customer->getCustomerId()));
            query.addBindValue(QString::fromStdString(customer->getName()));
            query.addBindValue(QString::fromStdString(customer->getPhoneNumber()));
            if (!query.exec()) {
                qDebug() << "Error saving Customer: " << query.lastError().text();
            }
        }
    }

    // 3. Lưu danh sách Room (Kiểm tra loại phòng để lưu đúng phí dịch vụ riêng)
    query.prepare("INSERT INTO Room (roomNumber, basePrice, isAvailable, roomType, premiumServiceFee, miniBarFee) VALUES (?, ?, ?, ?, ?, ?)");
    for (const auto& room : manager.getRooms()) {
        if (room) {
            query.addBindValue(QString::fromStdString(room->getRoomNumber()));
            query.addBindValue(room->getBasePrice());
            query.addBindValue(room->getIsAvailable() ? 1 : 0);

            // Xác định loại phòng và chi phí đi kèm bằng std::dynamic_pointer_cast
            double premiumFee = 0.0;
            double miniBarFee = 0.0;
            QString typeStr = "Standard";

            if (auto suite = std::dynamic_pointer_cast<SuiteRoom>(room)) {
                typeStr = "Suite";
                premiumFee = suite->getPremiumServiceFee();
            } else if (auto deluxe = std::dynamic_pointer_cast<DeluxeRoom>(room)) {
                typeStr = "Deluxe";
                miniBarFee = deluxe->getMiniBarFee();
            }

            query.addBindValue(typeStr);
            query.addBindValue(premiumFee);
            query.addBindValue(miniBarFee);

            if (!query.exec()) {
                qDebug() << "Error saving Room: " << query.lastError().text();
            }
        }
    }

    // 4. Lưu danh sách Booking
    query.prepare("INSERT INTO Booking (bookingId, customerId, roomNumber, checkInDate, checkOutDate) VALUES (?, ?, ?, ?, ?)");
    for (const auto& booking : manager.getBookings()) {
        if (booking && booking->getCustomer() && booking->getRoom()) {
            query.addBindValue(QString::fromStdString(booking->getBookingId()));
            query.addBindValue(QString::fromStdString(booking->getCustomer()->getCustomerId()));
            query.addBindValue(QString::fromStdString(booking->getRoom()->getRoomNumber()));

            // Chuyển đổi QDate trực tiếp sang QString theo định dạng ISO (YYYY-MM-DD)
            query.addBindValue(booking->getCheckInDate().toString(Qt::ISODate));
            query.addBindValue(booking->getCheckOutDate().toString(Qt::ISODate));

            if (!query.exec()) {
                qDebug() << "Error saving Booking: " << query.lastError().text();
            }
        }
    }

    return true;
}
