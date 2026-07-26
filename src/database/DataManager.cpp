#include "DataManager.h"
#include "SuiteRoom.h"
#include "DeluxeRoom.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QString>
#include <utility>

// Creates or returns the one shared DataManager instance.
// Reference helps ensuring only one DataManager instance exist
DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

bool DataManager::initDatabase(const std::string& dataPath) {
    m_dataPath = dataPath;

    if (m_db.isOpen() && m_db.databaseName() == QString::fromStdString(dataPath)) {
        return true;
    }

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qDebug() << "QSQLITE driver is not available. Please ensure the Qt SQLite plugin is deployed.";
        return false;
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(QString::fromStdString(dataPath));

    if (!m_db.open()) {
        qDebug() << "SQLite connection error: " << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;
    QSqlQuery schemaQuery;

    query.exec("PRAGMA foreign_keys = ON;");

    QString createCustomer =
        "CREATE TABLE IF NOT EXISTS Customer ("
        "   customerId TEXT PRIMARY KEY,"
        "   name TEXT NOT NULL,"
        "   phoneNumber TEXT,"
        "   archived INTEGER NOT NULL DEFAULT 0"
        ");";
    if (!query.exec(createCustomer)) {
        qDebug() << "Error creating Customer table: " << query.lastError().text();
        return false;
    }

    if (schemaQuery.exec("PRAGMA table_info(Customer)")) {
        bool hasArchived = false;
        while (schemaQuery.next()) {
            if (schemaQuery.value(1).toString() == "archived") {
                hasArchived = true;
            }
        }
        if (!hasArchived) {
            if (!schemaQuery.exec("ALTER TABLE Customer ADD COLUMN archived INTEGER NOT NULL DEFAULT 0")) {
                qDebug() << "Error adding archived column to Customer: " << schemaQuery.lastError().text();
                return false;
            }
        }
    } else {
        qDebug() << "Error inspecting Customer schema: " << schemaQuery.lastError().text();
        return false;
    }

    QString createRoom =
        "CREATE TABLE IF NOT EXISTS Room ("
        "   roomNumber TEXT PRIMARY KEY,"
        "   basePrice REAL NOT NULL,"
        "   isAvailable INTEGER DEFAULT 1,"
        "   archived INTEGER NOT NULL DEFAULT 0,"
        "   roomType TEXT NOT NULL,"
        "   premiumServiceFee REAL,"
        "   miniBarFee REAL"
        ");";
    if (!query.exec(createRoom)) {
        qDebug() << "Error creating Room table: " << query.lastError().text();
        return false;
    }

    if (schemaQuery.exec("PRAGMA table_info(Room)")) {
        bool hasArchived = false;
        while (schemaQuery.next()) {
            if (schemaQuery.value(1).toString() == "archived") {
                hasArchived = true;
            }
        }
        if (!hasArchived) {
            if (!schemaQuery.exec("ALTER TABLE Room ADD COLUMN archived INTEGER NOT NULL DEFAULT 0")) {
                qDebug() << "Error adding archived column to Room: " << schemaQuery.lastError().text();
                return false;
            }
        }
    } else {
        qDebug() << "Error inspecting Room schema: " << schemaQuery.lastError().text();
        return false;
    }

    QString createBooking =
        "CREATE TABLE IF NOT EXISTS Booking ("
        "   bookingId TEXT PRIMARY KEY,"
        "   customerId TEXT NOT NULL,"
        "   roomNumber TEXT NOT NULL,"
        "   checkInDate TEXT NOT NULL,"
        "   checkOutDate TEXT NOT NULL,"
        "   cancelled INTEGER NOT NULL DEFAULT 0,"
        "   deleted INTEGER NOT NULL DEFAULT 0,"
        "   checkedOut INTEGER NOT NULL DEFAULT 0,"
        "   FOREIGN KEY (customerId) REFERENCES Customer(customerId) ON DELETE CASCADE ON UPDATE CASCADE,"
        "   FOREIGN KEY (roomNumber) REFERENCES Room(roomNumber) ON DELETE RESTRICT ON UPDATE CASCADE"
        ");";
    if (!query.exec(createBooking)) {
        qDebug() << "Error creating Booking table: " << query.lastError().text();
        return false;
    }

    bool checkedOutColumnAdded = false;
    QSqlQuery bookingSchemaQuery;
    if (bookingSchemaQuery.exec("PRAGMA table_info(Booking)")) {
        bool hasCancelled = false;
        bool hasDeleted = false;
        bool hasCheckedOut = false;
        bool hasStatus = false;
        while (bookingSchemaQuery.next()) {
            const QString columnName = bookingSchemaQuery.value(1).toString();
            if (columnName == "cancelled") {
                hasCancelled = true;
            } else if (columnName == "deleted") {
                hasDeleted = true;
            } else if (columnName == "checkedOut") {
                hasCheckedOut = true;
            } else if (columnName == "status") {
                hasStatus = true;
            }
        }

        if (!hasCancelled && hasStatus) {
            if (!bookingSchemaQuery.exec("ALTER TABLE Booking ADD COLUMN cancelled INTEGER NOT NULL DEFAULT 0")) {
                qDebug() << "Error adding cancelled column: " << bookingSchemaQuery.lastError().text();
                return false;
            }

            QSqlQuery migrateQuery;
            if (!migrateQuery.exec("UPDATE Booking SET cancelled = CASE WHEN status = 'Canceled' OR status = 'Cancelled' THEN 1 ELSE 0 END")) {
                qDebug() << "Error migrating booking cancellation state: " << migrateQuery.lastError().text();
                return false;
            }
        }

        if (!hasDeleted) {
            if (!bookingSchemaQuery.exec("ALTER TABLE Booking ADD COLUMN deleted INTEGER NOT NULL DEFAULT 0")) {
                qDebug() << "Error adding deleted column: " << bookingSchemaQuery.lastError().text();
                return false;
            }
        }

        if (!hasCheckedOut) {
            if (!bookingSchemaQuery.exec("ALTER TABLE Booking ADD COLUMN checkedOut INTEGER NOT NULL DEFAULT 0")) {
                qDebug() << "Error adding checked-out column: " << bookingSchemaQuery.lastError().text();
                return false;
            }
            checkedOutColumnAdded = true;
        }
    } else {
        qDebug() << "Error inspecting Booking schema: " << bookingSchemaQuery.lastError().text();
        return false;
    }

    // Modified: Reconstructed schema to save atomic properties (taxRate, nights) instead of static calculated totalAmount
    QString createInvoice =
        "CREATE TABLE IF NOT EXISTS Invoice ("
        "   invoiceId TEXT PRIMARY KEY,"
        "   bookingId TEXT NOT NULL,"
        "   taxRate REAL NOT NULL,"
        "   nights INTEGER NOT NULL,"
        "   paymentDate TEXT NOT NULL,"
        "   FOREIGN KEY (bookingId) REFERENCES Booking(bookingId) ON DELETE CASCADE ON UPDATE CASCADE"
        ");";
    if (!query.exec(createInvoice)) {
        qDebug() << "Error creating Invoice table: " << query.lastError().text();
        return false;
    }

    if (checkedOutColumnAdded) {
        // Modified and optimized performance: preserve legacy completed stays once while moving future completion decisions to the explicit checkout flag.
        QSqlQuery migrateCompletedBookings;
        migrateCompletedBookings.prepare(
            "UPDATE Booking SET checkedOut = 1 "
            "WHERE cancelled = 0 AND deleted = 0 "
            "AND (checkOutDate <= ? OR bookingId IN (SELECT bookingId FROM Invoice))");
        migrateCompletedBookings.addBindValue(QDate::currentDate().toString(Qt::ISODate));
        if (!migrateCompletedBookings.exec()) {
            qDebug() << "Error migrating completed booking state: " << migrateCompletedBookings.lastError().text();
            return false;
        }
    }

    qDebug() << "The SQLite database is all set and ready!";
    return true;
}

// Loads application data from the configured storage location.
bool DataManager::loadAll(HotelManager& manager, const std::string& dataPath) {
    if (!initDatabase(dataPath)) {
        return false;
    }

    // Modified and optimized performance: stage the complete database load before replacing live state, preventing invalid rows from being silently dropped and later saved away.
    HotelManager loadedManager;

    // Modified: Removed legacy counter initializer since entity counters are managed dynamically
    QSqlQuery query;
    std::string errorMsg;

    // Load customer records from database into memory structures
    if (query.exec("SELECT customerId, name, phoneNumber, archived FROM Customer")) {
        while (query.next()) {
            std::string custId = query.value(0).toString().toStdString();
            std::string name = query.value(1).toString().toStdString();
            std::string phone = query.value(2).toString().toStdString();
            bool archived = query.value(3).toInt() == 1;

            if (!loadedManager.restoreCustomerFromDatabase(custId, name, phone, archived, errorMsg)) {
                qDebug() << "Failed to restore customer during load:" << QString::fromStdString(errorMsg);
                return false;
            }
        }
    } else {
        qDebug() << "Error reading Customer table: " << query.lastError().text();
        return false;
    }

    // Load room properties and restore distinct sub-type specific data fields
    if (query.exec("SELECT roomNumber, basePrice, isAvailable, archived, roomType, premiumServiceFee, miniBarFee FROM Room")) {
        while (query.next()) {
            std::string roomNum = query.value(0).toString().toStdString();
            double price = query.value(1).toDouble();
            int isAvailable = query.value(2).toInt();
            bool archived = query.value(3).toInt() == 1;
            std::string typeStr = query.value(4).toString().toStdString();
            double premiumFee = query.value(5).toDouble();
            double miniBarFee = query.value(6).toDouble();

            RoomType type = RoomType::Standard;
            if (typeStr == "Deluxe") type = RoomType::Deluxe;
            else if (typeStr == "Suite") type = RoomType::Suite;

            if (!loadedManager.registerRoom(type, roomNum, price, errorMsg)) {
                qDebug() << "Failed to restore room during load:" << QString::fromStdString(errorMsg);
                return false;
            }

            auto room = loadedManager.findRoomByNumber(roomNum);
            if (room) {
                room->setIsAvailable(isAvailable == 1);
                room->setArchived(archived);

                // Use std::dynamic_pointer_cast because the project utilizes std::shared_ptr instances
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

    // Fixed-modified: Rebuild the booking counter in the data layer instead of querying from the model.
    int maxBookingNumber = 1000;

    // Modified and optimized performance: load the explicit checkout flag so completed history is not inferred from dates.
    if (query.exec("SELECT bookingId, customerId, roomNumber, checkInDate, checkOutDate, cancelled, deleted, checkedOut FROM Booking")) {
        while (query.next()) {
            std::string bookingId = query.value(0).toString().toStdString();
            std::string custId = query.value(1).toString().toStdString();
            std::string roomNum = query.value(2).toString().toStdString();
            std::string checkInStr = query.value(3).toString().toStdString();
            std::string checkOutStr = query.value(4).toString().toStdString();
            bool cancelled = query.value(5).toInt() == 1;
            bool deleted = query.value(6).toInt() == 1;
            bool checkedOut = query.value(7).toInt() == 1;

            if (bookingId.rfind("BK", 0) == 0) {
                bool ok = false;
                const int numeric = QString::fromStdString(bookingId.substr(2)).toInt(&ok);
                if (ok && numeric > maxBookingNumber) {
                    maxBookingNumber = numeric;
                }
            }

            if (!loadedManager.restoreBookingFromDatabase(bookingId, custId, roomNum, checkInStr, checkOutStr, cancelled, deleted, checkedOut, errorMsg)) {
                qDebug() << "Failed to restore booking during load:" << QString::fromStdString(errorMsg);
                return false;
            }

        }
    } else {
        qDebug() << "Error reading Booking table: " << query.lastError().text();
        return false;
    }

    // Reconstruct invoices directly back into the core system memory.
    // Legacy rows with a cancelled flag are ignored by deleting them during migration.
    if (query.exec("SELECT invoiceId, bookingId, taxRate, nights, paymentDate FROM Invoice")) {
        while (query.next()) {
            std::string invId = query.value(0).toString().toStdString();
            std::string bookId = query.value(1).toString().toStdString();
            double tax = query.value(2).toDouble();
            int nightsCount = query.value(3).toInt();
            std::string payDate = query.value(4).toString().toStdString();

            if (!loadedManager.restoreInvoiceFromDatabase(invId, bookId, tax, nightsCount, payDate, errorMsg)) {
                qDebug() << "Failed to restore invoice during load:" << QString::fromStdString(errorMsg);
                return false;
            }
        }
    } else {
        qDebug() << "Error reading Invoice table: " << query.lastError().text();
        return false;
    }

    // Modified and optimized performance: publish only a fully validated snapshot and update the booking ID counter after a successful load.
    manager = std::move(loadedManager);
    Booking::initCounterFromDatabase(maxBookingNumber);
    return true;
}

// Modified: Removed const qualifier to allow weak_ptr locking mechanisms during data saving loops
//           saveAll() refuses to persist invalid object graphs instead of silently dropping broken bookings or invoices.
bool DataManager::saveAll(HotelManager& manager, const std::string& dataPath) {
    const std::string activePath = dataPath.empty() ? m_dataPath : dataPath;
    if (activePath.empty()) {
        qDebug() << "No database path available for saveAll.";
        return false;
    }

    if (!initDatabase(activePath)) {
        return false;
    }
    if (!m_db.transaction())
    {
        qDebug() << "Failed to start transaction: " << m_db.lastError().text();
        return false;
    } // Start a transaction to ensure atomicity of the save operation
    QSqlQuery query;

    // 1. Wipe old database entries completely before overwriting fresh model data records
    // Added: Clear out dependency table first
    if (!query.exec("DELETE FROM Invoice")) {
        m_db.rollback(); // Rollback transaction if anything happened that fails to maintain database integrity
        qDebug() << "Can't delete from Invoice: " << query.lastError().text();
        return false; // Always return false if the database is not in a valid state after a transaction, not allowing partial saves to occur
    } 
    if (!query.exec("DELETE FROM Booking")) {
        m_db.rollback(); 
        qDebug() << "Can't delete from Booking: " << query.lastError().text();
        return false;
    } 
    if (!query.exec("DELETE FROM Room")) {
        m_db.rollback(); 
        qDebug() << "Can't delete from Room: " << query.lastError().text();
        return false;
    } 
    if (!query.exec("DELETE FROM Customer")) {
        m_db.rollback(); 
        qDebug() << "Can't delete from Customer: " << query.lastError().text();
        return false;
    }

    // 2. Persist Customer object lists
    query.prepare("INSERT INTO Customer (customerId, name, phoneNumber, archived) VALUES (?, ?, ?, ?)");
    for (const auto& customer : manager.getCustomers()) {
        if (customer) {
            query.addBindValue(QString::fromStdString(customer->getCustomerId()));
            query.addBindValue(QString::fromStdString(customer->getName()));
            query.addBindValue(QString::fromStdString(customer->getPhoneNumber()));
            query.addBindValue(customer->isArchived() ? 1 : 0);
            if (!query.exec()) {
                m_db.rollback();
                qDebug() << "Error saving Customer: " << query.lastError().text();
                return false;
            }
        }
    }

    // 3. Persist Room lists (Inspect polymorphism variables to save correct individual fee items)
    query.prepare("INSERT INTO Room (roomNumber, basePrice, isAvailable, archived, roomType, premiumServiceFee, miniBarFee) VALUES (?, ?, ?, ?, ?, ?, ?)");
    for (const auto& room : manager.getRooms()) {
        if (room) {
            query.addBindValue(QString::fromStdString(room->getRoomNumber()));
            query.addBindValue(room->getBasePrice());
            query.addBindValue(room->getIsAvailable() ? 1 : 0);
            query.addBindValue(room->isArchived() ? 1 : 0);

            // Determine subtype structures and fee balances using dynamic casting pointers
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
                m_db.rollback();
                qDebug() << "Error saving Room: " << query.lastError().text();
                return false;
            }
        }
    }

    // Modified and optimized performance: persist checkout state with every atomic booking snapshot.
    query.prepare("INSERT INTO Booking (bookingId, customerId, roomNumber, checkInDate, checkOutDate, cancelled, deleted, checkedOut) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    for (const auto& booking : manager.getBookings()) {
        if (!booking) {
            continue;
        }

        auto customer = booking->getCustomer();
        auto room = booking->getRoom();
        if (!customer || !room) {
            m_db.rollback();
            qDebug() << "Invalid Booking object graph: booking references a missing customer or room.";
            return false;
        }

        query.addBindValue(QString::fromStdString(booking->getBookingId()));
        query.addBindValue(QString::fromStdString(customer->getCustomerId()));
        query.addBindValue(QString::fromStdString(room->getRoomNumber()));

        // Modified: Directly bind ISO strings instead of querying conversion method variants
        query.addBindValue(QString::fromStdString(booking->getCheckInDate()));
        query.addBindValue(QString::fromStdString(booking->getCheckOutDate()));

        query.addBindValue(booking->isCancelled() ? 1 : 0);
        query.addBindValue(booking->isDeleted() ? 1 : 0);
        query.addBindValue(booking->isCheckedOut() ? 1 : 0);

        if (!query.exec()) {
            m_db.rollback();
            qDebug() << "Error saving Booking: " << query.lastError().text();
            return false;
        }
    }

    // Added: 5. Persist Invoice records dynamically calculated up from view layer properties
    query.prepare("INSERT INTO Invoice (invoiceId, bookingId, taxRate, nights, paymentDate) VALUES (?, ?, ?, ?, ?)");
    for (const auto& invoice : manager.getInvoices()) {
        if (!invoice) {
            continue;
        }

        query.addBindValue(QString::fromStdString(invoice->getInvoiceId()));
        query.addBindValue(QString::fromStdString(invoice->getBookingId()));
        query.addBindValue(invoice->getTaxRate());
        query.addBindValue(invoice->getNights());
        query.addBindValue(QString::fromStdString(invoice->getPaymentDate()));
        if (!query.exec()) {
            m_db.rollback();
            qDebug() << "Error saving Invoice: " << query.lastError().text();
            return false;
        }
    }
    if (!m_db.commit())
    {
        m_db.rollback();
        qDebug() << "Failed to commit transaction: " << m_db.lastError().text();
        return false;
    }
    return true;
}

bool DataManager::restoreLastSavedState(HotelManager& manager)
{
    if (m_dataPath.empty()) {
        qDebug() << "No database path available for restoring the last saved state.";
        return false;
    }

    return loadAll(manager, m_dataPath);
}

bool DataManager::commitChanges(HotelManager& manager)
{
    // Modified and optimized performance: keep the in-memory model synchronized with the last successful SQLite transaction.
    if (saveAll(manager)) {
        return true;
    }

    qDebug() << "Save failed; restoring the in-memory model from the last committed database state.";
    restoreLastSavedState(manager);
    return false;
}
