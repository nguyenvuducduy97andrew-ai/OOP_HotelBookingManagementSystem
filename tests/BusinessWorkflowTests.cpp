#include "BookingManager.h"
#include "DataManager.h"
#include "HotelManager.h"
#include "RoomManager.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QThread>

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

class TestFailure final : public std::runtime_error
{
public:
    explicit TestFailure(const std::string& message) : std::runtime_error(message) {}
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw TestFailure(message);
    }
}

std::string iso(const QDateTime& value)
{
    return value.toString(Qt::ISODateWithMs).toStdString();
}

QDateTime futureAt(int daysFromToday, int hour, int minute = 0)
{
    return QDateTime(QDate::currentDate().addDays(daysFromToday), QTime(hour, minute));
}

void seedHotel(HotelManager& manager)
{
    std::string error;
    require(manager.registerCustomer("100000000001", "Alex Stone", "+84912345678", error), error);
    require(manager.registerCustomer("100000000002", "Jamie Rivers", "+84912345679", error), error);
    require(manager.registerRoom(RoomType::Standard, "101", 100000.0, 24.0, "Queen bed", 3,
                                 "Business room", "Wi-Fi", error), error);
    require(manager.registerRoom(RoomType::Deluxe, "102", 200000.0, 32.0, "King bed", 4,
                                 "Deluxe room", "Wi-Fi, TV", error), error);
}

std::string createFutureBooking(HotelManager& manager, const std::string& customerId,
                                const std::string& roomNumber, const QDateTime& start,
                                const QDateTime& end)
{
    std::string error;
    require(manager.createBookingAt(customerId, roomNumber, iso(start), iso(end), 1, 0, error), error);
    require(!manager.getBookings().empty(), "The successful booking was not retained.");
    return manager.getBookings().back()->getBookingId();
}

void testSameDayAndOvernightBookings()
{
    HotelManager manager;
    seedHotel(manager);

    createFutureBooking(manager, "100000000001", "101", futureAt(4, 10), futureAt(4, 12));
    createFutureBooking(manager, "100000000002", "102", futureAt(5, 22), futureAt(6, 2));

    require(manager.getBookings().size() == 2, "Same-day and overnight reservations must both be accepted.");
}

void testAdjacentSlotsRespectTurnoverBuffer()
{
    HotelManager manager;
    seedHotel(manager);
    const QDateTime firstStart = futureAt(6, 10);
    const QDateTime firstEnd = futureAt(6, 12);
    createFutureBooking(manager, "100000000001", "101", firstStart, firstEnd);

    createFutureBooking(manager, "100000000002", "101", firstEnd.addSecs(2 * 60 * 60), firstEnd.addSecs(3 * 60 * 60));

    std::string error;
    require(!manager.createBookingAt("100000000002", "101", iso(firstEnd.addSecs(2 * 60 * 60 - 60)),
                                     iso(firstEnd.addSecs(3 * 60 * 60)), 1, 0, error),
            "A booking beginning before the two-hour turnover boundary must be rejected.");
}

void testCleaningBlocksAndEarlyRoomReady()
{
    RoomManager rooms;
    std::string error;
    require(rooms.registerRoom(RoomType::Standard, "201", 100000.0, 24.0, "Queen bed", 2,
                               "Test room", "Wi-Fi", error), error);

    const QDateTime checkout = futureAt(7, 10);
    require(rooms.startCleaningAfterCheckout("201", iso(checkout), error), error);
    require(rooms.isRoomBlockedAt("201", iso(checkout.addSecs(90 * 60))),
            "Cleaning must block the room during its default two-hour period.");

    require(rooms.markRoomReady("201", iso(checkout.addSecs(30 * 60)), "Test staff", error), error);
    require(rooms.isRoomBlockedAt("201", iso(checkout.addSecs(15 * 60))),
            "The room must stay blocked until the recorded early-ready time.");
    require(!rooms.isRoomBlockedAt("201", iso(checkout.addSecs(31 * 60))),
            "Marking a room ready must release Cleaning early.");
}

void testMaintenanceIsFullDayAndBlocksBooking()
{
    HotelManager manager;
    seedHotel(manager);
    const QDate firstDay = QDate::currentDate().addDays(8);
    const QDate exclusiveEnd = firstDay.addDays(2); // The UI converts the selected inclusive end to this exclusive boundary.
    std::string error;
    require(manager.scheduleRoomMaintenance("101", firstDay.toString(Qt::ISODate).toStdString(),
                                            exclusiveEnd.toString(Qt::ISODate).toStdString(),
                                            "Annual inspection", error), error);

    require(manager.isRoomBlockedAt("101", iso(QDateTime(firstDay, QTime(0, 0)))),
            "Maintenance must block the selected start day.");
    require(manager.isRoomBlockedAt("101", iso(QDateTime(firstDay.addDays(1), QTime(23, 59)))),
            "Maintenance must block the selected inclusive end day.");

    require(!manager.createBookingAt("100000000001", "101", iso(QDateTime(firstDay, QTime(10, 0))),
                                     iso(QDateTime(firstDay, QTime(12, 0))), 1, 0, error),
            "A planned booking overlapping confirmed maintenance must be rejected.");
}

void testEarlyCheckInEarlyCheckoutAndActiveMaintenanceGuard()
{
    HotelManager manager;
    seedHotel(manager);

    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime plannedStart = now.addSecs(60 * 60);
    const QDateTime plannedEnd = plannedStart.addSecs(3 * 60 * 60);
    const std::string bookingId = createFutureBooking(manager, "100000000001", "101", plannedStart, plannedEnd);

    std::string error;
    require(manager.checkInBookingAt(bookingId, iso(now), error), error);
    require(manager.getBookingState(*manager.findBookingById(bookingId)) == BookingState::ACTIVE,
            "Early same-day arrival must become an Active stay after staff check-in.");

    require(!manager.scheduleRoomMaintenance("101", QDate::currentDate().toString(Qt::ISODate).toStdString(),
                                            QDate::currentDate().addDays(1).toString(Qt::ISODate).toStdString(),
                                            "Unsafe active overlap", error),
            "Maintenance must not be scheduled over an active stay.");

    QThread::sleep(1);
    require(manager.completeBookingAt(bookingId, iso(QDateTime::currentDateTime()), error), error);
    require(manager.getBookingState(*manager.findBookingById(bookingId)) == BookingState::COMPLETED,
            "A checkout before planned departure must complete the active stay.");
    require(manager.isRoomBlockedAt("101", manager.findBookingById(bookingId)->getActualCheckOutAt()),
            "Checkout must immediately begin the Cleaning block.");
}

void testHourlyBillingRoundingAndMinimum()
{
    require(BookingManager::calculateBillableHours(29 * 60) == 1,
            "29 minutes must keep the one-hour minimum.");
    require(BookingManager::calculateBillableHours(30 * 60) == 1,
            "30 minutes must round to one billable hour.");
    require(BookingManager::calculateBillableHours(31 * 60) == 1,
            "31 minutes must remain one billable hour.");
    require(BookingManager::calculateBillableHours(2 * 60 * 60 + 29 * 60) == 2,
            "2 hours 29 minutes must round down to two billable hours.");
    require(BookingManager::calculateBillableHours(2 * 60 * 60 + 30 * 60) == 3,
            "2 hours 30 minutes must round up to three billable hours.");
    require(BookingManager::calculateBillableHours(2 * 60 * 60 + 31 * 60) == 3,
            "2 hours 31 minutes must remain three billable hours.");
}

void testRoomChangeSelfExclusionCancellationAndRateSnapshot()
{
    HotelManager manager;
    seedHotel(manager);
    const QDateTime start = futureAt(10, 10);
    const QDateTime end = futureAt(10, 12);
    const std::string bookingId = createFutureBooking(manager, "100000000001", "101", start, end);
    const auto booking = manager.findBookingById(bookingId);
    require(booking && booking->getQuotedHourlyRate() == 100000.0, "The booking must capture the original room rate.");

    std::string error;
    require(manager.updateBookingAt(bookingId, "100000000001", "101", iso(start), iso(end), 1, 0, error), error);
    require(manager.updateBookingAt(bookingId, "100000000001", "102", iso(start), iso(end), 1, 0, error), error);
    require(booking->getRoom()->getRoomNumber() == "102", "A successful edit must change the assigned room.");

    require(manager.updateRoomDetails("102", 300000.0, 50000.0, 32.0, "King bed", 4,
                                      "Changed later", "Wi-Fi, TV", error), error);
    require(booking->getQuotedHourlyRate() == 200000.0,
            "The booking's confirmed base rate must remain unchanged after later room-price or optional-fee edits.");

    require(manager.cancelBooking(bookingId, "Guest cancelled", error), error);
    require(manager.getBookingState(*booking) == BookingState::CANCELLED,
            "Cancelling an upcoming reservation must retain it as Cancelled history.");
}

void testExistingAndNewCustomerFlowsDoNotCreateDuplicatesOrOrphans()
{
    HotelManager manager;
    seedHotel(manager);
    std::string error;
    require(manager.resolveCustomerForBooking("100000000001", "Alex Stone", "+84912345678", error), error);

    const QDateTime blockedStart = futureAt(11, 10);
    const QDateTime blockedEnd = futureAt(11, 12);
    createFutureBooking(manager, "100000000001", "101", blockedStart, blockedEnd);
    const std::size_t initialCustomers = manager.getCustomers().size();

    require(!manager.createBookingForNewCustomer("100000000003", "Morgan Lee", "+84912345680", "101",
                                                 iso(blockedStart), iso(blockedEnd), 1, 0, error),
            "A colliding new-customer booking must fail.");
    require(manager.getCustomers().size() == initialCustomers && !manager.findCustomerById("100000000003"),
            "A failed new-customer booking must not leave an orphan customer.");

    require(!manager.createBookingForNewCustomer("100000000001", "Alex Stone", "+84912345678", "102",
                                                 iso(futureAt(12, 10)), iso(futureAt(12, 12)), 1, 0, error),
            "The new-customer flow must reject an existing identity instead of duplicating it.");

    require(manager.createBookingForNewCustomer("100000000004", "Morgan Lee", "+84912345681", "102",
                                                iso(futureAt(12, 10)), iso(futureAt(12, 12)), 1, 0, error), error);
    require(manager.findCustomerById("100000000004") != nullptr,
            "A successful new-customer booking must retain its customer.");
}

void testLegacyDatabaseMigration()
{
    QTemporaryDir temporaryDirectory;
    require(temporaryDirectory.isValid(), "Cannot create the temporary database directory.");
    const QString databasePath = QDir(temporaryDirectory.path()).filePath("legacy-hotel.db");

    {
        QSqlDatabase legacy = QSqlDatabase::addDatabase("QSQLITE", "legacy-seed");
        legacy.setDatabaseName(databasePath);
        require(legacy.open(), "Cannot open the legacy database seed file.");
        QSqlQuery query(legacy);
        require(query.exec("CREATE TABLE Customer (customerId TEXT PRIMARY KEY, name TEXT NOT NULL, phoneNumber TEXT)"),
                query.lastError().text().toStdString());
        require(query.exec("INSERT INTO Customer(customerId, name, phoneNumber) VALUES "
                           "('100000000009', 'Legacy Guest', '+84912345689')"),
                query.lastError().text().toStdString());
        legacy.close();
    }
    QSqlDatabase::removeDatabase("legacy-seed");

    HotelManager restored;
    require(DataManager::getInstance().loadAll(restored, databasePath.toStdString()),
            "The legacy database must migrate and load successfully.");
    const auto customer = restored.findCustomerById("100000000009");
    require(customer != nullptr && customer->getIssuingCountry() == "Legacy",
            "Migration must preserve the legacy customer and mark its legacy issuing country.");

    QSqlQuery columns;
    require(columns.exec("PRAGMA table_info(Customer)"), "Cannot inspect migrated Customer columns.");
    bool hasDocumentType = false;
    bool hasIssuingCountry = false;
    bool hasDocumentNumber = false;
    while (columns.next()) {
        const QString name = columns.value(1).toString();
        hasDocumentType = hasDocumentType || name == "documentType";
        hasIssuingCountry = hasIssuingCountry || name == "issuingCountry";
        hasDocumentNumber = hasDocumentNumber || name == "documentNumber";
    }
    require(hasDocumentType && hasIssuingCountry && hasDocumentNumber,
            "Migration must add the current customer identity columns.");
}

int seedDemoDatabase(const std::string& databasePath)
{
    HotelManager manager;
    if (!DataManager::getInstance().loadAll(manager, databasePath)) {
        std::cerr << "[FAIL] Could not load database for demo seeding: " << databasePath << '\n';
        return 1;
    }
    if (!manager.getCustomers().empty() || !manager.getRooms().empty() || !manager.getBookings().empty()
        || !manager.getInvoices().empty() || !manager.getRoomMaintenances().empty()) {
        std::cerr << "[FAIL] Demo data is inserted only into an empty database to protect existing records.\n";
        return 1;
    }

    const auto requireSeed = [](bool result, const std::string& error) {
        if (!result) {
            throw TestFailure(error.empty() ? "A demo operation failed." : error);
        }
    };

    try {
        std::string error;
        // Modified: Seed representative operational states through the real facade so demo rows exercise the same business rules as staff actions.
        requireSeed(manager.registerCustomer("100000000001", "Nguyen Minh Khoi", "+84912345678", error), error);
        requireSeed(manager.registerCustomer("100000000002", "Le Quang Khai", "+84912345679", error), error);
        requireSeed(manager.registerCustomer("100000000003", "Jamie Rivers", "+14155550128", error), error);
        requireSeed(manager.registerCustomer("100000000004", "Aisha Rahman", "+60123456789", error), error);
        requireSeed(manager.registerCustomer("100000000005", "Lee Wang", "+6581234567", error), error);

        requireSeed(manager.registerRoom(RoomType::Standard, "101", 100000.0, 25.0, "Queen bed", 2,
                                         "Quiet standard room for short business stays.", "Wi-Fi, Air conditioning", error), error);
        requireSeed(manager.registerRoom(RoomType::Deluxe, "102", 180000.0, 34.0, "King bed", 3,
                                         "Spacious deluxe room with city view.", "Wi-Fi, Smart TV, Minibar", error), error);
        requireSeed(manager.registerRoom(RoomType::Suite, "103", 300000.0, 52.0, "Super King bed", 4,
                                         "Suite reserved for maintenance scheduling demonstrations.", "Wi-Fi, Smart TV, Bathtub", error), error);
        requireSeed(manager.registerRoom(RoomType::Standard, "104", 100000.0, 26.0, "Twin beds", 2,
                                         "Active-stay demonstration room.", "Wi-Fi, Air conditioning", error), error);
        requireSeed(manager.registerRoom(RoomType::Deluxe, "105", 220000.0, 36.0, "King bed", 3,
                                         "Checkout and automatic Cleaning demonstration room.", "Wi-Fi, Smart TV", error), error);
        requireSeed(manager.registerRoom(RoomType::Suite, "106", 320000.0, 55.0, "Super King bed", 4,
                                         "Maintenance soft-hold demonstration room.", "Wi-Fi, Smart TV, Bathtub", error), error);

        const QDateTime now = QDateTime::currentDateTime();
        const auto create = [&](const std::string& customerId, const std::string& roomNumber,
                                const QDateTime& start, const QDateTime& end) {
            requireSeed(manager.createBookingAt(customerId, roomNumber, iso(start), iso(end), 1, 0, error), error);
            return manager.getBookings().back()->getBookingId();
        };

        create("100000000001", "101", futureAt(1, 10), futureAt(1, 13));
        const std::string cancelledBooking = create("100000000002", "102", futureAt(2, 14), futureAt(2, 17));
        requireSeed(manager.cancelBooking(cancelledBooking, "Guest changed travel plans", error), error);

        const QDateTime activePlannedStart = now.addSecs(60 * 60);
        const std::string activeBooking = create("100000000003", "104", activePlannedStart,
                                                 activePlannedStart.addSecs(4 * 60 * 60));
        requireSeed(manager.checkInBookingAt(activeBooking, iso(now), error), error);

        const QDateTime completedPlannedStart = now.addSecs(60 * 60);
        const std::string completedBooking = create("100000000004", "105", completedPlannedStart,
                                                    completedPlannedStart.addSecs(3 * 60 * 60));
        requireSeed(manager.checkInBookingAt(completedBooking, iso(now), error), error);
        QThread::sleep(1);
        requireSeed(manager.completeBookingAt(completedBooking, iso(QDateTime::currentDateTime()), error), error);
        const auto completed = manager.findBookingById(completedBooking);
        requireSeed(completed != nullptr, "Completed demo booking was not found.");
        const double completedTotal = completed->getQuotedHourlyRate() * (1.0 + completed->getQuotedTaxRate());
        requireSeed(manager.createInvoice(manager.nextInvoiceId(), completedBooking,
                                          QDate::currentDate().toString(Qt::ISODate).toStdString(),
                                          "Cash", completedTotal,
                                          QDate::currentDate().toString(Qt::ISODate).toStdString(), error), error);

        const QDate maintenanceStart = QDate::currentDate().addDays(5);
        requireSeed(manager.scheduleRoomMaintenance("103", maintenanceStart.toString(Qt::ISODate).toStdString(),
                                                    maintenanceStart.addDays(2).toString(Qt::ISODate).toStdString(),
                                                    "Annual safety inspection", error), error);

        create("100000000005", "106", futureAt(8, 10), futureAt(8, 13));
        const QDate softHoldStart = QDate::currentDate().addDays(8);
        requireSeed(manager.scheduleRoomMaintenance("106", softHoldStart.toString(Qt::ISODate).toStdString(),
                                                    softHoldStart.addDays(2).toString(Qt::ISODate).toStdString(),
                                                    "Demo maintenance requiring guest coordination", error), error);

        requireSeed(DataManager::getInstance().commitChanges(manager),
                    "Could not persist the demo data to the database.");
        std::cout << "[PASS] Seeded demo database: " << databasePath << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "[FAIL] Demo seed failed: " << exception.what() << '\n';
        return 1;
    }
}

int run(const std::string& name, const std::function<void()>& test)
{
    try {
        test();
        std::cout << "[PASS] " << name << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "[FAIL] " << name << ": " << exception.what() << '\n';
        return 1;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == "--initialize-database") {
        // Modified: Reuse the production DataManager migration path to create an empty, current-schema test database without demo rows.
        HotelManager emptyManager;
        const std::string databasePath = QString::fromLocal8Bit(argv[2]).toStdString();
        if (!DataManager::getInstance().loadAll(emptyManager, databasePath)) {
            std::cerr << "[FAIL] Could not initialize database: " << databasePath << '\n';
            return 1;
        }
        std::cout << "[PASS] Initialized empty database: " << databasePath << '\n';
        return 0;
    }
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == "--seed-demo-database") {
        // Modified: Allow a guarded local demo seed for classroom verification without bypassing production validation or persistence.
        return seedDemoDatabase(QString::fromLocal8Bit(argv[2]).toStdString());
    }

    int failures = 0;
    failures += run("same-day and overnight bookings", testSameDayAndOvernightBookings);
    failures += run("adjacent slots", testAdjacentSlotsRespectTurnoverBuffer);
    failures += run("cleaning and early room ready", testCleaningBlocksAndEarlyRoomReady);
    failures += run("full-day maintenance", testMaintenanceIsFullDayAndBlocksBooking);
    failures += run("early check-in and checkout", testEarlyCheckInEarlyCheckoutAndActiveMaintenanceGuard);
    failures += run("hourly rounding", testHourlyBillingRoundingAndMinimum);
    failures += run("room change, self exclusion, cancellation and rate snapshot", testRoomChangeSelfExclusionCancellationAndRateSnapshot);
    failures += run("existing and new customer flows", testExistingAndNewCustomerFlowsDoNotCreateDuplicatesOrOrphans);
    failures += run("legacy database migration", testLegacyDatabaseMigration);
    return failures == 0 ? 0 : 1;
}
