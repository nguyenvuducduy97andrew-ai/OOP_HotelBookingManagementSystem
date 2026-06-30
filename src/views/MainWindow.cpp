#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

MainWindow::MainWindow(HotelManager* mgr, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , controller(mgr) // Assign the core controller
{
    ui->setupUi(this);

    // Map UI pointer variables from the .ui design file to class properties
    roomTable = ui->roomTable;
    tabWidget = ui->tabWidget;

    // Immediately display vacant/occupied room status on the grid when the app opens
    updateRoomGrid();
}

MainWindow::~MainWindow() {
    delete ui;
}

// Update data from the controller (HotelManager) and dump it directly onto the public QTableWidget
void MainWindow::updateRoomGrid() {
    if (!roomTable) return;

    // Clear old rows to load entirely fresh data
    roomTable->setRowCount(0);

    const auto& rooms = controller->getRooms();
    for (size_t i = 0; i < rooms.size(); ++i) {
        if (!rooms[i]) continue;

        int row = roomTable->rowCount();
        roomTable->insertRow(row);

        // Column 0: Room Number
        roomTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(rooms[i]->getRoomNumber())));

        // Column 1: Polymorphic calculated price (Standard, Deluxe, Suite calculate rates based on room type)
        roomTable->setItem(row, 1, new QTableWidgetItem(QString::number(rooms[i]->calculateTargetPrice(), 'f', 2)));

        // Column 2: Room Status
        QString status = rooms[i]->getIsAvailable() ? "Available" : "Booked";
        roomTable->setItem(row, 2, new QTableWidgetItem(status));
    }
}

// Added: Helper method to compute stay duration using Qt's QDate utility, acting as an MVC bridge
int MainWindow::getDurationInNights(const std::string& checkInStr, const std::string& checkOutStr) const {
    // Convert core std::string back to QDate temporarily for UI/View-level calendar calculation
    QDate checkIn = QDate::fromString(QString::fromStdString(checkInStr), Qt::ISODate);
    QDate checkOut = QDate::fromString(QString::fromStdString(checkOutStr), Qt::ISODate);

    // Safety guard for invalid inputs or checkout dates that are before check-in dates
    if (!checkIn.isValid() || !checkOut.isValid() || checkOut <= checkIn) {
        return 0;
    }

    // Leverage QDate's robust daysTo method to return the pure integer count of nights
    return static_cast<int>(checkIn.daysTo(checkOut));
}

// Handle booking logic when the user clicks the button
void MainWindow::on_btnBook_clicked() {
    // Gather dummy input data from input fields on your Form UI
    std::string customerId = ui->txtCustomerId->text().toStdString();
    std::string roomNumber = ui->txtRoomNumber->text().toStdString();

    // Fetch QDate format from the UI calendar widget
    QDate checkIn = ui->dateCheckIn->date();
    QDate checkOut = ui->dateCheckOut->date();

    std::string errorMsg;

    // Modified: Converted QDate objects into ISO strings for checkOverbooking compatibility
    std::string checkInStr = checkIn.toString(Qt::ISODate).toStdString();
    std::string checkOutStr = checkOut.toString(Qt::ISODate).toStdString();

    // 1. Check for booking schedule conflicts (Perfectly matches the checkOverbooking method in the diagram)
    if (!controller->checkOverbooking(roomNumber, checkInStr, checkOutStr)) {
        QMessageBox::warning(this, "Overbooking Error", "This room has already been booked within the specified timeframe!");
        return;
    }

    // 2. Proceed to push the room registration command down to the Core control layer
    // Note: createBooking will call internal addBooking(...) to store the schedule
    bool success = controller->createBooking(customerId, roomNumber, checkInStr, checkOutStr, errorMsg);

    if (success) {
        QMessageBox::information(this, "Success", "Room booked successfully!");
        updateRoomGrid(); // Redraw vacant/occupied room UI
    } else {
        QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg));
    }
}

// Modified: Refactored checkout handler to dynamically process stay duration and trigger decoupled invoice creation
void MainWindow::on_btnCheckout_clicked() {
    std::string roomNumber = ui->txtCheckoutRoomNum->text().toStdString();

    // Added: Find active booking related to the checked-out room to retrieve dates
    std::shared_ptr<Booking> activeBooking = nullptr;
    for (const auto& b : controller->getBookings()) {
        if (b && b->getRoom() && b->getRoom()->getRoomNumber() == roomNumber && !b->getRoom()->getIsAvailable()) {
            activeBooking = b;
            break;
        }
    }

    // Call the checkoutRoom function to drop the BOOKED -> AVAILABLE status under core HotelManager layer
    controller->checkoutRoom(roomNumber);

    // Added: Generate decoupled billing if booking reference exists
    if (activeBooking) {
        // Compute nights dynamically at the View level
        int nights = getDurationInNights(activeBooking->getCheckInDate(), activeBooking->getCheckOutDate());

        // Build timestamp and next safe ID values
        std::string paymentDate = QDate::currentDate().toString(Qt::ISODate).toStdString();
        std::string invId = controller->nextInvoiceId();
        std::string errorMsg;
        double flatTaxRate = 0.1; // 10% standard hotel service tax

        // Inject pre-calculated variables down into core invoice logic
        if (!controller->createInvoice(invId, activeBooking->getBookingId(), flatTaxRate, nights, paymentDate, errorMsg)) {
            qDebug() << "Invoice generation failed during checkout:" << QString::fromStdString(errorMsg);
        }
    }

    QMessageBox::information(this, "Checkout Success", "Checkout successfully processed and invoice generated!");
    updateRoomGrid(); // Update vacant/occupied room colors/status on the screen
}