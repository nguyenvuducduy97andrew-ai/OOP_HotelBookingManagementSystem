#include <iostream>
#include <iomanip>
#include <string>

#include "HotelManager.h"
#include "DataManager.h"

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void printSeparator() {
    std::cout << std::string(60, '-') << std::endl;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << std::fixed << std::setprecision(2);

    // Initialize the hotel management system
    HotelManager manager;
    std::string errorMsg;

    printHeader("HOTEL BOOKING MANAGEMENT SYSTEM - DEMO");

    // ========== STEP 1: Add Rooms ==========
    printHeader("STEP 1: Adding Rooms");

    if (manager.addRoomIfValid(RoomKind::Standard, 101, 120.0, errorMsg)) {
        std::cout << "✓ Room 101 (Standard, $120/night) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add Room 101: " << errorMsg << std::endl;
    }

    if (manager.addRoomIfValid(RoomKind::Standard, 102, 120.0, errorMsg)) {
        std::cout << "✓ Room 102 (Standard, $120/night) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add Room 102: " << errorMsg << std::endl;
    }

    if (manager.addRoomIfValid(RoomKind::Deluxe, 201, 200.0, errorMsg)) {
        std::cout << "✓ Room 201 (Deluxe, $200/night) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add Room 201: " << errorMsg << std::endl;
    }

    if (manager.addRoomIfValid(RoomKind::Suite, 301, 350.0, errorMsg)) {
        std::cout << "✓ Room 301 (Suite, $350/night) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add Room 301: " << errorMsg << std::endl;
    }

    std::cout << "\nTotal rooms added: " << manager.getRooms().size() << std::endl;

    // ========== STEP 2: Add Customers ==========
    printHeader("STEP 2: Adding Customers");

    if (manager.addCustomerIfValid("C001", "Alice Johnson", "555-1001", errorMsg)) {
        std::cout << "✓ Customer C001 (Alice Johnson) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add customer: " << errorMsg << std::endl;
    }

    if (manager.addCustomerIfValid("C002", "Bob Smith", "555-1002", errorMsg)) {
        std::cout << "✓ Customer C002 (Bob Smith) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add customer: " << errorMsg << std::endl;
    }

    if (manager.addCustomerIfValid("C003", "Carol Davis", "555-1003", errorMsg)) {
        std::cout << "✓ Customer C003 (Carol Davis) added successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to add customer: " << errorMsg << std::endl;
    }

    std::cout << "\nTotal customers registered: " << manager.getCustomers().size() << std::endl;

    // ========== STEP 3: Create Bookings ==========
    printHeader("STEP 3: Creating Bookings");

    // Booking 1: Alice in Room 101 for 3 nights
    if (manager.addBookingIfValid("BK001", "C001", 101, "2024-06-25", "2024-06-28", errorMsg)) {
        std::cout << "✓ Booking BK001 created" << std::endl;
        std::cout << "  Customer: Alice Johnson" << std::endl;
        std::cout << "  Room: 101 (Standard)" << std::endl;
        std::cout << "  Dates: 2024-06-25 to 2024-06-28 (3 nights)" << std::endl;
    } else {
        std::cout << "✗ Failed to create booking: " << errorMsg << std::endl;
    }

    // Booking 2: Bob in Room 201 for 2 nights
    if (manager.addBookingIfValid("BK002", "C002", 201, "2024-06-26", "2024-06-28", errorMsg)) {
        std::cout << "✓ Booking BK002 created" << std::endl;
        std::cout << "  Customer: Bob Smith" << std::endl;
        std::cout << "  Room: 201 (Deluxe)" << std::endl;
        std::cout << "  Dates: 2024-06-26 to 2024-06-28 (2 nights)" << std::endl;
    } else {
        std::cout << "✗ Failed to create booking: " << errorMsg << std::endl;
    }

    // Booking 3: Carol in Room 301 for 4 nights
    if (manager.addBookingIfValid("BK003", "C003", 301, "2024-06-27", "2024-07-01", errorMsg)) {
        std::cout << "✓ Booking BK003 created" << std::endl;
        std::cout << "  Customer: Carol Davis" << std::endl;
        std::cout << "  Room: 301 (Suite)" << std::endl;
        std::cout << "  Dates: 2024-06-27 to 2024-07-01 (4 nights)" << std::endl;
    } else {
        std::cout << "✗ Failed to create booking: " << errorMsg << std::endl;
    }

    std::cout << "\nTotal bookings created: " << manager.getBookings().size() << std::endl;

    // ========== STEP 4: Display Room Status ==========
    printHeader("STEP 4: Room Availability Status");

    const auto& allRooms = manager.getRooms();
    for (const auto& room : allRooms) {
        if (room) {
            std::cout << "Room " << room->getRoomNumber();
            std::cout << " (" << (room->getIsAvailable() ? "AVAILABLE" : "BOOKED") << ")";
            std::cout << " - Base Price: $" << room->getBasePrice() << "/night" << std::endl;
        }
    }

    // ========== STEP 5: Create Invoices ==========
    printHeader("STEP 5: Creating Invoices");

    const int taxRate = 0;  // 0% tax for demo

    // Invoice for Booking BK001
    auto invoice1 = manager.createInvoice("INV001", "BK001", 3, taxRate, errorMsg);
    if (invoice1) {
        std::cout << "✓ Invoice INV001 created" << std::endl;
        std::cout << "  Booking: BK001 (Alice, Room 101)" << std::endl;
        std::cout << "  Duration: 3 nights × $120 = $" << invoice1->getTotalAmount() << std::endl;
    } else {
        std::cout << "✗ Failed to create invoice: " << errorMsg << std::endl;
    }

    // Invoice for Booking BK002
    auto invoice2 = manager.createInvoice("INV002", "BK002", 2, taxRate, errorMsg);
    if (invoice2) {
        std::cout << "✓ Invoice INV002 created" << std::endl;
        std::cout << "  Booking: BK002 (Bob, Room 201)" << std::endl;
        std::cout << "  Duration: 2 nights × $200 = $" << invoice2->getTotalAmount() << std::endl;
    } else {
        std::cout << "✗ Failed to create invoice: " << errorMsg << std::endl;
    }

    // Invoice for Booking BK003
    auto invoice3 = manager.createInvoice("INV003", "BK003", 4, taxRate, errorMsg);
    if (invoice3) {
        std::cout << "✓ Invoice INV003 created" << std::endl;
        std::cout << "  Booking: BK003 (Carol, Room 301)" << std::endl;
        std::cout << "  Duration: 4 nights × $350 = $" << invoice3->getTotalAmount() << std::endl;
    } else {
        std::cout << "✗ Failed to create invoice: " << errorMsg << std::endl;
    }

    // ========== STEP 6: Display Summary ==========
    printHeader("STEP 6: System Summary");

    std::cout << "Total Rooms: " << manager.getRooms().size() << std::endl;
    std::cout << "Available Rooms: " << manager.getAvailableRooms().size() << std::endl;
    std::cout << "Total Customers: " << manager.getCustomers().size() << std::endl;
    std::cout << "Total Bookings: " << manager.getBookings().size() << std::endl;

    double totalRevenue = 0.0;
    if (invoice1) totalRevenue += invoice1->getTotalAmount();
    if (invoice2) totalRevenue += invoice2->getTotalAmount();
    if (invoice3) totalRevenue += invoice3->getTotalAmount();
    std::cout << "Total Revenue: $" << totalRevenue << std::endl;

    // ========== STEP 7: Demonstrate Data Persistence ==========
    printHeader("STEP 7: Data Persistence");

    DataManager& dataManager = DataManager::getInstance();
    if (dataManager.saveAll("hotel_data")) {
        std::cout << "✓ System data saved successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to save system data" << std::endl;
    }

    if (dataManager.loadAll("hotel_data")) {
        std::cout << "✓ System data loaded successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to load system data" << std::endl;
    }

    printHeader("DEMO COMPLETED");
    std::cout << "Hotel Booking Management System is ready for use!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    return 0;
}
