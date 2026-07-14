#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDate>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <algorithm>

MainWindow::MainWindow(HotelManager* mgr, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , controller(mgr) // Assign the core controller
{
    ui->setupUi(this);

    // Map UI pointer variables from the .ui design file to class properties
    roomTable = ui->roomTable;
    tabWidget = ui->tabWidget;

    // Immediately display and populate data across all interfaces when the app opens
    refreshAllViews();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::refreshAllViews()
{
    updateRoomGrid();
    updateBookingGrids();
    updateCustomerGrid();
    updateDashboard();
}

// Update data from the controller (HotelManager) and dump it directly onto the public QTableWidget
void MainWindow::updateRoomGrid() {
    if (!roomTable) return;

    // Clear old rows to load entirely fresh data
    roomTable->setRowCount(0);

    const auto& rooms = controller->getRooms();
    const auto availableRooms = controller->getAvailableRooms();
    const auto occupiedRooms = controller->getRoomsByOccupancy(true);
    std::vector<std::string> availableRoomNumbers;
    availableRoomNumbers.reserve(availableRooms.size());
    for (const auto& availableRoom : availableRooms) {
        if (availableRoom) {
            availableRoomNumbers.push_back(availableRoom->getRoomNumber());
        }
    }

    std::vector<std::string> occupiedRoomNumbers;
    occupiedRoomNumbers.reserve(occupiedRooms.size());
    for (const auto& occupiedRoom : occupiedRooms) {
        if (occupiedRoom) {
            occupiedRoomNumbers.push_back(occupiedRoom->getRoomNumber());
        }
    }

    for (size_t i = 0; i < rooms.size(); ++i) {
        if (!rooms[i]) continue;

        int row = roomTable->rowCount();
        roomTable->insertRow(row);

        // Column 0: Room Number
        roomTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(rooms[i]->getRoomNumber())));

        // Column 1: Polymorphic calculated price (Standard, Deluxe, Suite calculate rates based on room type)
        roomTable->setItem(row, 1, new QTableWidgetItem(QString::number(rooms[i]->calculateTargetPrice(), 'f', 2)));

        // Column 2: Room Status
        QString status = "Available";
        if (!rooms[i]->getIsAvailable()) {
            status = "Maintenance";
        } else if (std::find(occupiedRoomNumbers.begin(), occupiedRoomNumbers.end(), rooms[i]->getRoomNumber()) != occupiedRoomNumbers.end()) {
            status = "Occupied";
        } else if (std::find(availableRoomNumbers.begin(), availableRoomNumbers.end(), rooms[i]->getRoomNumber()) == availableRoomNumbers.end()) {
            status = "Reserved";
        }
        roomTable->setItem(row, 2, new QTableWidgetItem(status));
    }
}

// Added: Redraws all filtered booking records on UI tables based on their operational status
void MainWindow::updateBookingGrids() {
    // Note: Assuming your .ui has corresponding tables for each state, or a shared generic booking table (e.g., ui->bookingTable)
    if (!ui->bookingTable) return;

    ui->bookingTable->setRowCount(0);
    const auto& allBookings = controller->getBookings();

    for (const auto& b : allBookings) {
        if (!b) continue;

        int row = ui->bookingTable->rowCount();
        ui->bookingTable->insertRow(row);

        ui->bookingTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(b->getBookingId())));
        ui->bookingTable->setItem(row, 1, new QTableWidgetItem(b->getCustomer() ? QString::fromStdString(b->getCustomer()->getCustomerId()) : "N/A"));
        ui->bookingTable->setItem(row, 2, new QTableWidgetItem(b->getRoom() ? QString::fromStdString(b->getRoom()->getRoomNumber()) : "N/A"));
        ui->bookingTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(b->getCheckInDate())));
        ui->bookingTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(b->getCheckOutDate())));
        const BookingState status = controller->getBookingState(*b);
        const std::string statusText = status == BookingState::UPCOMING
            ? "Upcoming (Reserved)"
            : status == BookingState::ACTIVE
                ? "Active (Checked-in)"
                : status == BookingState::COMPLETED
                    ? "Completed (Checked-out)"
                    : "Cancelled (Voided)";
        ui->bookingTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(statusText)));
    }
}

// Added: Refresh customer directories into View tables
void MainWindow::updateCustomerGrid() {
    if (!ui->customerTable) return;

    ui->customerTable->setRowCount(0);
    const auto& customers = controller->getCustomers();

    for (const auto& c : customers) {
        if (!c) continue;

        int row = ui->customerTable->rowCount();
        ui->customerTable->insertRow(row);

        ui->customerTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c->getCustomerId())));
        ui->customerTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c->getName())));
        ui->customerTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c->getPhoneNumber())));
    }
}

// Added: Computes metrics based on today's date context and dumps them onto Dashboard KPI widgets
void MainWindow::updateDashboard() {
    std::string todayStr = QDate::currentDate().toString(Qt::ISODate).toStdString();

    auto arrivals = controller->getArrivalsByDate(todayStr);
    auto departures = controller->getDeparturesByDate(todayStr);
    auto availableRooms = controller->getAvailableRooms();

    // Mapping variables onto text labels or list elements inside your layout design view
    if (ui->lblTodayArrivals) ui->lblTodayArrivals->setText(QString::number(arrivals.size()));
    if (ui->lblTodayDepartures) ui->lblTodayDepartures->setText(QString::number(departures.size()));
    if (ui->lblAvailableRooms) ui->lblAvailableRooms->setText(QString::number(availableRooms.size()));
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

    std::string checkInStr = checkIn.toString(Qt::ISODate).toStdString();
    std::string checkOutStr = checkOut.toString(Qt::ISODate).toStdString();

    // 1. Proceed to push the room registration command down to the Core control layer
    // Note: The system internally uses isRoomFreeForDates which ignores Canceled bookings!
    bool success = controller->createBooking(customerId, roomNumber, checkInStr, checkOutStr, errorMsg);

    if (success) {
        QMessageBox::information(this, "Success", "Room booked successfully!");
        refreshAllViews();
    } else {
        QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg));
    }
}

// Modified: Refactored checkout handler to dynamically process stay duration and trigger decoupled invoice creation
void MainWindow::on_btnCheckout_clicked() {
    std::string bookingId = ui->txtCheckoutBookingId->text().toStdString(); // Better MVC design using BookingID
    std::shared_ptr<Booking> activeBooking = controller->findBookingById(bookingId);

    if (!activeBooking) {
        QMessageBox::warning(this, "Checkout Error", "Specified Booking ID record could not be found!");
        return;
    }

    if (controller->getBookingState(*activeBooking) != BookingState::ACTIVE) {
        QMessageBox::warning(this, "Checkout Error", "Only Active (Checked-in) bookings can be checked-out!");
        return;
    }

    // Compute nights dynamically at the View level
    int nights = getDurationInNights(activeBooking->getCheckInDate(), activeBooking->getCheckOutDate());
    if (nights == 0) nights = 1; // Minimum baseline compensation enforcement

    // Build timestamp and next safe ID values
    std::string paymentDate = QDate::currentDate().toString(Qt::ISODate).toStdString();
    std::string invId = controller->nextInvoiceId();
    std::string errorMsg;
    double flatTaxRate = 0.1; // 10% standard hotel service tax

    // Inject pre-calculated variables down into core invoice logic.
    // Core layer will auto-update Booking state to Completed and release the Room entity inside!
    if (controller->createInvoice(invId, activeBooking->getBookingId(), flatTaxRate, nights, paymentDate, errorMsg)) {
        QMessageBox::information(this, "Checkout Success", "Checkout processed and Invoice " + QString::fromStdString(invId) + " generated successfully!");
        refreshAllViews();
    } else {
        QMessageBox::critical(this, "Error", "Invoice generation failed: " + QString::fromStdString(errorMsg));
    }
}

// Added: Soft-cancels a reservation through the underlying domain controller wrapper without scrubbing logs
void MainWindow::on_btnCancelBooking_clicked() {
    std::string bookingId = ui->txtCancelBookingId->text().toStdString();
    std::string errorMsg;

    if (controller->cancelBooking(bookingId, errorMsg)) {
        QMessageBox::information(this, "Canceled Success", "The booking reservation has been successfully canceled!");
        refreshAllViews();
    } else {
        QMessageBox::critical(this, "Cancellation Error", QString::fromStdString(errorMsg));
    }
}

// Added: Capture table selection changes to implement dynamic customer CRM history lookup view layouts
void MainWindow::on_customerTable_itemSelectionChanged() {
    int currentRow = ui->customerTable->currentRow();
    if (currentRow < 0 || !ui->customerHistoryTable) return;

    std::string customerId = ui->customerTable->item(currentRow, 0)->text().toStdString();
    auto trackingLogs = controller->getBookingsForCustomer(customerId);

    ui->customerHistoryTable->setRowCount(0);
    for (const auto& b : trackingLogs) {
        if (!b) continue;
        int r = ui->customerHistoryTable->rowCount();
        ui->customerHistoryTable->insertRow(r);

        ui->customerHistoryTable->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(b->getBookingId())));
        ui->customerHistoryTable->setItem(r, 1, new QTableWidgetItem(b->getRoom() ? QString::fromStdString(b->getRoom()->getRoomNumber()) : "N/A"));
        ui->customerHistoryTable->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(b->getCheckInDate() + " to " + b->getCheckOutDate())));
        ui->customerHistoryTable->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(bookingStateToString(controller->getBookingState(*b)))));
    }
}

// Added: Global visual refresh dispatcher synchronized when shifting focus across app tab indexes
void MainWindow::on_tabWidget_currentChanged(int index) {
    Q_UNUSED(index);
    refreshAllViews();
}