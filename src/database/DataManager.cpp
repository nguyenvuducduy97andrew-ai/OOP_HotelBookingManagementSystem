#include "DataManager.h"
#include "SuiteRoom.h"
#include "DeluxeRoom.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QString>
#include <utility>
#include <vector>

// Creates or returns the one shared DataManager instance.
// Reference helps ensuring only one DataManager instance exist
DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

bool DataManager::readDatabaseRevision(qint64& revision) const
{
    QSqlQuery query(m_db);
    if (!query.exec("SELECT revision FROM DataVersion WHERE id = 1") || !query.next()) {
        qDebug() << "Error reading database revision:" << query.lastError().text();
        return false;
    }
    revision = query.value(0).toLongLong();
    return true;
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

    // Modified: Verify foreign-key enforcement and wait briefly for transient SQLite locks instead of continuing unsafely.
    if (!query.exec("PRAGMA foreign_keys = ON;") || !query.exec("PRAGMA busy_timeout = 5000;")) {
        qDebug() << "Error configuring SQLite connection:" << query.lastError().text();
        return false;
    }
    if (!query.exec("PRAGMA foreign_keys;") || !query.next() || query.value(0).toInt() != 1) {
        qDebug() << "SQLite foreign-key enforcement could not be enabled:" << query.lastError().text();
        return false;
    }

    // Modified: Maintain a revision row so a stale in-memory snapshot cannot overwrite another application's save.
    if (!query.exec("CREATE TABLE IF NOT EXISTS DataVersion (id INTEGER PRIMARY KEY CHECK(id = 1), revision INTEGER NOT NULL)") ||
        !query.exec("INSERT OR IGNORE INTO DataVersion (id, revision) VALUES (1, 0)")) {
        qDebug() << "Error creating database revision metadata:" << query.lastError().text();
        return false;
    }

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

    QString createRoomMaintenance =
        "CREATE TABLE IF NOT EXISTS RoomMaintenance ("
        "   maintenanceId TEXT PRIMARY KEY,"
        "   roomNumber TEXT NOT NULL,"
        "   startDate TEXT NOT NULL,"
        "   endDate TEXT NOT NULL,"
        "   note TEXT,"
        "   FOREIGN KEY (roomNumber) REFERENCES Room(roomNumber) ON DELETE CASCADE ON UPDATE CASCADE"
        ");";
    if (!query.exec(createRoomMaintenance)) {
        qDebug() << "Error creating RoomMaintenance table:" << query.lastError().text();
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

    // Modified: Persist immutable billing snapshots so later room and customer edits cannot alter historical invoices.
    QString createInvoice =
        "CREATE TABLE IF NOT EXISTS Invoice ("
        "   invoiceId TEXT PRIMARY KEY,"
        "   bookingId TEXT NOT NULL,"
        "   taxRate REAL NOT NULL,"
        "   nights INTEGER NOT NULL,"
        "   paymentDate TEXT NOT NULL,"
        "   unitPrice REAL NOT NULL DEFAULT 0,"
        "   customerNameSnapshot TEXT NOT NULL DEFAULT '',"
        "   customerIdSnapshot TEXT NOT NULL DEFAULT '',"
        "   customerPhoneSnapshot TEXT NOT NULL DEFAULT '',"
        "   roomNumberSnapshot TEXT NOT NULL DEFAULT '',"
        "   roomTypeSnapshot TEXT NOT NULL DEFAULT '',"
        "   checkInDateSnapshot TEXT NOT NULL DEFAULT '',"
        "   checkOutDateSnapshot TEXT NOT NULL DEFAULT '',"
        "   FOREIGN KEY (bookingId) REFERENCES Booking(bookingId) ON DELETE CASCADE ON UPDATE CASCADE"
        ");";
    if (!query.exec(createInvoice)) {
        qDebug() << "Error creating Invoice table: " << query.lastError().text();
        return false;
    }

    const auto ensureInvoiceColumn = [&schemaQuery](const QString& columnName, const QString& definition) {
        if (!schemaQuery.exec("PRAGMA table_info(Invoice)")) {
            qDebug() << "Error inspecting Invoice schema:" << schemaQuery.lastError().text();
            return false;
        }
        while (schemaQuery.next()) {
            if (schemaQuery.value(1).toString() == columnName) {
                return true;
            }
        }
        if (!schemaQuery.exec("ALTER TABLE Invoice ADD COLUMN " + definition)) {
            qDebug() << "Error adding Invoice snapshot column:" << schemaQuery.lastError().text();
            return false;
        }
        return true;
    };
    const std::vector<std::pair<QString, QString>> invoiceColumns = {
        {"unitPrice", "unitPrice REAL NOT NULL DEFAULT 0"},
        {"customerNameSnapshot", "customerNameSnapshot TEXT NOT NULL DEFAULT ''"},
        {"customerIdSnapshot", "customerIdSnapshot TEXT NOT NULL DEFAULT ''"},
        {"customerPhoneSnapshot", "customerPhoneSnapshot TEXT NOT NULL DEFAULT ''"},
        {"roomNumberSnapshot", "roomNumberSnapshot TEXT NOT NULL DEFAULT ''"},
        {"roomTypeSnapshot", "roomTypeSnapshot TEXT NOT NULL DEFAULT ''"},
        {"checkInDateSnapshot", "checkInDateSnapshot TEXT NOT NULL DEFAULT ''"},
        {"checkOutDateSnapshot", "checkOutDateSnapshot TEXT NOT NULL DEFAULT ''"}
    };
    for (const auto& [columnName, definition] : invoiceColumns) {
        if (!ensureInvoiceColumn(columnName, definition)) {
            return false;
        }
    }

    QSqlQuery snapshotMigration(m_db);
    // Modified: Backfill legacy invoices once from their linked records before enforcing immutable invoice loading.
    if (!snapshotMigration.exec(
            "UPDATE Invoice SET "
            "unitPrice = COALESCE((SELECT r.basePrice + CASE r.roomType WHEN 'Deluxe' THEN COALESCE(r.miniBarFee, 0) WHEN 'Suite' THEN COALESCE(r.premiumServiceFee, 0) ELSE 0 END FROM Booking b JOIN Room r ON r.roomNumber = b.roomNumber WHERE b.bookingId = Invoice.bookingId), unitPrice), "
            "customerNameSnapshot = COALESCE((SELECT c.name FROM Booking b JOIN Customer c ON c.customerId = b.customerId WHERE b.bookingId = Invoice.bookingId), customerNameSnapshot), "
            "customerIdSnapshot = COALESCE((SELECT b.customerId FROM Booking b WHERE b.bookingId = Invoice.bookingId), customerIdSnapshot), "
            "customerPhoneSnapshot = COALESCE((SELECT c.phoneNumber FROM Booking b JOIN Customer c ON c.customerId = b.customerId WHERE b.bookingId = Invoice.bookingId), customerPhoneSnapshot), "
            "roomNumberSnapshot = COALESCE((SELECT b.roomNumber FROM Booking b WHERE b.bookingId = Invoice.bookingId), roomNumberSnapshot), "
            "roomTypeSnapshot = COALESCE((SELECT r.roomType FROM Booking b JOIN Room r ON r.roomNumber = b.roomNumber WHERE b.bookingId = Invoice.bookingId), roomTypeSnapshot), "
            "checkInDateSnapshot = COALESCE((SELECT b.checkInDate FROM Booking b WHERE b.bookingId = Invoice.bookingId), checkInDateSnapshot), "
            "checkOutDateSnapshot = COALESCE((SELECT b.checkOutDate FROM Booking b WHERE b.bookingId = Invoice.bookingId), checkOutDateSnapshot) "
            "WHERE unitPrice <= 0 OR customerNameSnapshot = '' OR customerIdSnapshot = '' OR customerPhoneSnapshot = '' OR roomNumberSnapshot = '' OR roomTypeSnapshot = '' OR checkInDateSnapshot = '' OR checkOutDateSnapshot = ''")) {
        qDebug() << "Error backfilling Invoice snapshots:" << snapshotMigration.lastError().text();
        return false;
    }

    if (checkedOutColumnAdded) {
        // Modified: Mark legacy bookings completed only when an invoice proves checkout occurred.
        QSqlQuery migrateCompletedBookings;
        migrateCompletedBookings.prepare(
            "UPDATE Booking SET checkedOut = 1 "
            "WHERE cancelled = 0 AND deleted = 0 "
            "AND bookingId IN (SELECT bookingId FROM Invoice)");
        if (!migrateCompletedBookings.exec()) {
            qDebug() << "Error migrating completed booking state: " << migrateCompletedBookings.lastError().text();
            return false;
        }
    }

    const QStringList indexStatements = {
        "CREATE INDEX IF NOT EXISTS idx_booking_room_dates ON Booking(roomNumber, checkInDate, checkOutDate)",
        "CREATE INDEX IF NOT EXISTS idx_booking_customer ON Booking(customerId)",
        "CREATE INDEX IF NOT EXISTS idx_maintenance_room_dates ON RoomMaintenance(roomNumber, startDate, endDate)"
    };
    for (const QString& statement : indexStatements) {
        if (!query.exec(statement)) {
            qDebug() << "Error creating performance index:" << query.lastError().text();
            return false;
        }
    }

    if (!reconcileCustomerDuplicates()) {
        return false;
    }
    // Modified: Enforce the customer phone uniqueness rule in SQLite after safe legacy reconciliation.
    if (!query.exec("DROP INDEX IF EXISTS idx_customer_phone") ||
        !query.exec("CREATE UNIQUE INDEX IF NOT EXISTS uq_customer_phone ON Customer(phoneNumber) WHERE phoneNumber IS NOT NULL AND phoneNumber <> ''")) {
        qDebug() << "Error enforcing unique customer phone numbers:" << query.lastError().text();
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

    // Modified: Stage the complete database load before replacing live state to prevent invalid rows from being silently dropped and later saved away.
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

    // Modified: Rebuild the booking counter in the data layer instead of querying from the model.
    int maxBookingNumber = 1000;

    // Modified: Load the explicit checkout flag so completed history is not inferred from dates.
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

    if (query.exec("SELECT maintenanceId, roomNumber, startDate, endDate, note FROM RoomMaintenance")) {
        while (query.next()) {
            const std::string maintenanceId = query.value(0).toString().toStdString();
            const std::string roomNumber = query.value(1).toString().toStdString();
            const std::string startDate = query.value(2).toString().toStdString();
            const std::string endDate = query.value(3).toString().toStdString();
            const std::string note = query.value(4).toString().toStdString();

            if (!loadedManager.restoreRoomMaintenanceFromDatabase(
                    maintenanceId, roomNumber, startDate, endDate, note, errorMsg)) {
                qDebug() << "Failed to restore room maintenance during load:" << QString::fromStdString(errorMsg);
                return false;
            }
        }
    } else {
        qDebug() << "Error reading RoomMaintenance table:" << query.lastError().text();
        return false;
    }

    // Reconstruct invoices directly back into the core system memory.
    // Legacy rows with a cancelled flag are ignored by deleting them during migration.
    if (query.exec("SELECT invoiceId, bookingId, taxRate, nights, paymentDate, unitPrice, customerNameSnapshot, customerIdSnapshot, customerPhoneSnapshot, roomNumberSnapshot, roomTypeSnapshot, checkInDateSnapshot, checkOutDateSnapshot FROM Invoice")) {
        while (query.next()) {
            std::string invId = query.value(0).toString().toStdString();
            std::string bookId = query.value(1).toString().toStdString();
            double tax = query.value(2).toDouble();
            int nightsCount = query.value(3).toInt();
            std::string payDate = query.value(4).toString().toStdString();
            double unitPrice = query.value(5).toDouble();
            std::string customerNameSnapshot = query.value(6).toString().toStdString();
            std::string customerIdSnapshot = query.value(7).toString().toStdString();
            std::string customerPhoneSnapshot = query.value(8).toString().toStdString();
            std::string roomNumberSnapshot = query.value(9).toString().toStdString();
            std::string roomTypeSnapshot = query.value(10).toString().toStdString();
            std::string checkInDateSnapshot = query.value(11).toString().toStdString();
            std::string checkOutDateSnapshot = query.value(12).toString().toStdString();

            if (!loadedManager.restoreInvoiceFromDatabase(invId, bookId, tax, nightsCount, payDate, unitPrice,
                                                          customerNameSnapshot, customerIdSnapshot, customerPhoneSnapshot,
                                                          roomNumberSnapshot, roomTypeSnapshot, checkInDateSnapshot,
                                                          checkOutDateSnapshot, errorMsg)) {
                qDebug() << "Failed to restore invoice during load:" << QString::fromStdString(errorMsg);
                return false;
            }
        }
    } else {
        qDebug() << "Error reading Invoice table: " << query.lastError().text();
        return false;
    }

    // Modified: Publish only a fully validated snapshot and update the booking ID counter after a successful load.
    qint64 loadedRevision = -1;
    if (!readDatabaseRevision(loadedRevision)) {
        return false;
    }
    manager = std::move(loadedManager);
    Booking::initCounterFromDatabase(maxBookingNumber);
    m_loadedRevision = loadedRevision;
    return true;
}

bool DataManager::reconcileCustomerDuplicates()
{
    struct CanonicalCustomer {
        qint64 rowId;
        QString customerId;
    };

    QSqlQuery selectQuery(m_db);
    if (!selectQuery.exec("SELECT rowid, customerId, name, phoneNumber FROM Customer ORDER BY archived ASC, rowid ASC")) {
        qDebug() << "Failed to scan customers for duplicate reconciliation:" << selectQuery.lastError().text();
        return false;
    }

    struct CustomerRow {
        qint64 rowId;
        QString customerId;
        QString normalizedName;
        QString normalizedPhone;
    };

    std::vector<CustomerRow> customerRows;
    while (selectQuery.next()) {
        CustomerRow row {
            selectQuery.value(0).toLongLong(),
            selectQuery.value(1).toString().trimmed(),
            selectQuery.value(2).toString().simplified().toCaseFolded(),
            selectQuery.value(3).toString().simplified()
        };
        row.normalizedPhone.remove(' ');
        customerRows.push_back(std::move(row));
    }
    selectQuery.finish();

    QHash<QString, QString> nameByPhone;
    for (const CustomerRow& row : customerRows) {
        if (row.normalizedPhone.isEmpty() || row.normalizedName.isEmpty()) {
            continue;
        }
        const auto knownName = nameByPhone.constFind(row.normalizedPhone);
        if (knownName != nameByPhone.cend() && *knownName != row.normalizedName) {
            // Modified: Stop on ambiguous shared-phone records instead of deleting data that cannot be identified safely.
            qDebug() << "Ambiguous duplicate customer phone requires manual review:" << row.normalizedPhone;
            return false;
        }
        nameByPhone.insert(row.normalizedPhone, row.normalizedName);
    }

    QHash<QString, qint64> knownCustomerRows;
    bool duplicatesDetected = false;
    for (const CustomerRow& row : customerRows) {
        if (row.customerId.isEmpty() || row.normalizedName.isEmpty() || row.normalizedPhone.isEmpty()) {
            continue;
        }

        const QString identityKey = row.normalizedName + QChar(0x1F) + row.normalizedPhone;
        if (knownCustomerRows.contains(identityKey)) {
            duplicatesDetected = true;
            break;
        }
        knownCustomerRows.insert(identityKey, row.rowId);
    }

    if (!duplicatesDetected) {
        return true;
    }

    // Modified: Create a durable database backup before reconciliation deletes duplicate customer rows.
    QSqlQuery checkpointQuery(m_db);
    if (!checkpointQuery.exec("PRAGMA wal_checkpoint(FULL)")) {
        qDebug() << "Failed to checkpoint database before customer duplicate backup:"
                 << checkpointQuery.lastError().text();
        return false;
    }

    const QString databasePath = m_db.databaseName();
    const QString backupPath = databasePath + ".customer-dedup-"
        + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmsszzz") + ".bak";
    if (databasePath.isEmpty() || !QFile::exists(databasePath) || !QFile::copy(databasePath, backupPath)) {
        qDebug() << "Failed to create customer duplicate reconciliation backup:" << backupPath;
        return false;
    }
    qDebug() << "Created customer duplicate reconciliation backup:" << backupPath;

    if (!m_db.transaction()) {
        qDebug() << "Failed to begin customer duplicate reconciliation:" << m_db.lastError().text();
        return false;
    }

    QHash<QString, CanonicalCustomer> canonicalCustomers;
    int mergedCount = 0;
    for (const CustomerRow& row : customerRows) {
        // Modified: Keep incomplete historical rows separate because their identity cannot be verified safely.
        if (row.customerId.isEmpty() || row.normalizedName.isEmpty() || row.normalizedPhone.isEmpty()) {
            continue;
        }

        const QString identityKey = row.normalizedName + QChar(0x1F) + row.normalizedPhone;
        const auto canonical = canonicalCustomers.constFind(identityKey);
        if (canonical == canonicalCustomers.cend()) {
            canonicalCustomers.insert(identityKey, {row.rowId, row.customerId});
            continue;
        }

        // Modified: Merge only an exact normalized name-and-phone match, then repoint historical bookings before removing the duplicate row.
        if (row.customerId != canonical->customerId) {
            QSqlQuery repointBookings(m_db);
            repointBookings.prepare("UPDATE Booking SET customerId = ? WHERE customerId = ?");
            repointBookings.addBindValue(canonical->customerId);
            repointBookings.addBindValue(row.customerId);
            if (!repointBookings.exec()) {
                m_db.rollback();
                qDebug() << "Failed to repoint duplicate customer bookings:" << repointBookings.lastError().text();
                return false;
            }
        }

        QSqlQuery deleteCustomer(m_db);
        deleteCustomer.prepare("DELETE FROM Customer WHERE rowid = ?");
        deleteCustomer.addBindValue(row.rowId);
        if (!deleteCustomer.exec()) {
            m_db.rollback();
            qDebug() << "Failed to delete duplicate customer:" << deleteCustomer.lastError().text();
            return false;
        }

        ++mergedCount;
    }

    if (mergedCount > 0) {
        // Modified: Publish duplicate reconciliation as a database revision so other running instances cannot save stale snapshots.
        QSqlQuery revisionQuery(m_db);
        if (!revisionQuery.exec("UPDATE DataVersion SET revision = revision + 1 WHERE id = 1")) {
            m_db.rollback();
            qDebug() << "Failed to advance revision after duplicate reconciliation:" << revisionQuery.lastError().text();
            return false;
        }
    }
    if (!m_db.commit()) {
        m_db.rollback();
        qDebug() << "Failed to commit customer duplicate reconciliation:" << m_db.lastError().text();
        return false;
    }

    if (mergedCount > 0) {
        qDebug() << "Reconciled duplicate customer records:" << mergedCount;
    }
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

    qint64 currentRevision = -1;
    if (!query.exec("SELECT revision FROM DataVersion WHERE id = 1") || !query.next()) {
        m_db.rollback();
        qDebug() << "Failed to read database revision before save:" << query.lastError().text();
        return false;
    }
    currentRevision = query.value(0).toLongLong();
    // Modified: Reject a stale full snapshot before it can overwrite changes saved by another application instance.
    if (m_loadedRevision >= 0 && m_loadedRevision != currentRevision) {
        m_db.rollback();
        qDebug() << "Database changed since this application last loaded it; refusing stale snapshot save.";
        return false;
    }

    // 1. Wipe old database entries completely before overwriting fresh model data records
    // Modified: Clear dependent records before replacing the database snapshot.
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
    if (!query.exec("DELETE FROM RoomMaintenance")) {
        m_db.rollback();
        qDebug() << "Can't delete from RoomMaintenance:" << query.lastError().text();
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

    // Modified: Persist dated maintenance intervals separately from permanent room availability.
    query.prepare("INSERT INTO RoomMaintenance (maintenanceId, roomNumber, startDate, endDate, note) VALUES (?, ?, ?, ?, ?)");
    for (const RoomMaintenance& maintenance : manager.getRoomMaintenances()) {
        query.addBindValue(QString::fromStdString(maintenance.getMaintenanceId()));
        query.addBindValue(QString::fromStdString(maintenance.getRoomNumber()));
        query.addBindValue(QString::fromStdString(maintenance.getStartDate()));
        query.addBindValue(QString::fromStdString(maintenance.getEndDate()));
        query.addBindValue(QString::fromStdString(maintenance.getNote()));
        if (!query.exec()) {
            m_db.rollback();
            qDebug() << "Error saving RoomMaintenance:" << query.lastError().text();
            return false;
        }
    }

    // Modified: Persist checkout state with every atomic booking snapshot.
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

    // Modified: Persist immutable invoice snapshots with every financial record.
    query.prepare("INSERT INTO Invoice (invoiceId, bookingId, taxRate, nights, paymentDate, unitPrice, customerNameSnapshot, customerIdSnapshot, customerPhoneSnapshot, roomNumberSnapshot, roomTypeSnapshot, checkInDateSnapshot, checkOutDateSnapshot) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    for (const auto& invoice : manager.getInvoices()) {
        if (!invoice) {
            continue;
        }
        if (!invoice->isValid()) {
            m_db.rollback();
            qDebug() << "Invalid Invoice object graph: immutable billing snapshot is incomplete.";
            return false;
        }

        query.addBindValue(QString::fromStdString(invoice->getInvoiceId()));
        query.addBindValue(QString::fromStdString(invoice->getBookingId()));
        query.addBindValue(invoice->getTaxRate());
        query.addBindValue(invoice->getNights());
        query.addBindValue(QString::fromStdString(invoice->getPaymentDate()));
        query.addBindValue(invoice->getUnitPrice());
        query.addBindValue(QString::fromStdString(invoice->getCustomerNameSnapshot()));
        query.addBindValue(QString::fromStdString(invoice->getCustomerIdSnapshot()));
        query.addBindValue(QString::fromStdString(invoice->getCustomerPhoneSnapshot()));
        query.addBindValue(QString::fromStdString(invoice->getRoomNumberSnapshot()));
        query.addBindValue(QString::fromStdString(invoice->getRoomTypeSnapshot()));
        query.addBindValue(QString::fromStdString(invoice->getCheckInDateSnapshot()));
        query.addBindValue(QString::fromStdString(invoice->getCheckOutDateSnapshot()));
        if (!query.exec()) {
            m_db.rollback();
            qDebug() << "Error saving Invoice: " << query.lastError().text();
            return false;
        }
    }
    if (!query.exec("UPDATE DataVersion SET revision = revision + 1 WHERE id = 1")) {
        m_db.rollback();
        qDebug() << "Failed to advance database revision:" << query.lastError().text();
        return false;
    }
    if (!m_db.commit())
    {
        m_db.rollback();
        qDebug() << "Failed to commit transaction: " << m_db.lastError().text();
        return false;
    }
    m_loadedRevision = currentRevision + 1;
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
    // Modified: Keep the in-memory model synchronized with the last successful SQLite transaction.
    if (saveAll(manager)) {
        return true;
    }

    qDebug() << "Save failed; restoring the in-memory model from the last committed database state.";
    restoreLastSavedState(manager);
    return false;
}
