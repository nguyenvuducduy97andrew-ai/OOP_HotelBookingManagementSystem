#include <iostream>
#include <iomanip>
#include <string>

#include "HotelManager.h"
#include "DataManager.h"

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

void printSeparator() {
    std::cout << std::string(70, '-') << std::endl;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << std::fixed << std::setprecision(2);

    // Initialize the hotel management system
    HotelManager manager;
    std::string errorMsg;

    printHeader("HOTEL BOOKING MANAGEMENT SYSTEM - UPDATED DEMO");
    std::cout << "Demonstrating OOP Design: Encapsulation, Inheritance, Polymorphism, Abstraction" << std::endl;

    // ========== STEP 1: Register Rooms ==========
    printHeader("STEP 1: Room Registration (RoomFactory Pattern)");

    if (manager.registerRoom(RoomType::Standard, "101", 120.0, errorMsg)) {
        std::cout << "✓ Room 101 (Standard, $120/night) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register Room 101: " << errorMsg << std::endl;
    }

    if (manager.registerRoom(RoomType::Standard, "102", 120.0, errorMsg)) {
        std::cout << "✓ Room 102 (Standard, $120/night) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register Room 102: " << errorMsg << std::endl;
    }

    if (manager.registerRoom(RoomType::Deluxe, "201", 200.0, errorMsg)) {
        std::cout << "✓ Room 201 (Deluxe, $200/night) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register Room 201: " << errorMsg << std::endl;
    }

    if (manager.registerRoom(RoomType::Suite, "301", 350.0, errorMsg)) {
        std::cout << "✓ Room 301 (Suite, $350/night) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register Room 301: " << errorMsg << std::endl;
    }

    std::cout << "\nTotal rooms registered: " << manager.getRooms().size() << std::endl;

    // ========== STEP 2: Register Customers ==========
    printHeader("STEP 2: Customer Registration with Validation");

    if (manager.registerCustomer("C001", "Alice Johnson", "555-1001", errorMsg)) {
        std::cout << "✓ Customer C001 (Alice Johnson) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register customer: " << errorMsg << std::endl;
    }

    if (manager.registerCustomer("C002", "Bob Smith", "555-1002", errorMsg)) {
        std::cout << "✓ Customer C002 (Bob Smith) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register customer: " << errorMsg << std::endl;
    }

    if (manager.registerCustomer("C003", "Carol Davis", "555-1003", errorMsg)) {
        std::cout << "✓ Customer C003 (Carol Davis) registered successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to register customer: " << errorMsg << std::endl;
    }

    std::cout << "\nTotal customers registered: " << manager.getCustomers().size() << std::endl;

    // ========== STEP 3: Create Bookings ==========
    printHeader("STEP 3: Booking Creation with Date Validation");

    // Booking 1: Alice in Room 101 for 3 nights
    if (manager.createBooking("C001", "101", "2024-06-25", "2024-06-28", errorMsg)) {
        std::cout << "✓ Booking created for Alice Johnson" << std::endl;
        std::cout << "  Room: 101 (Standard)" << std::endl;
        std::cout << "  Duration: 2024-06-25 to 2024-06-28 (3 nights)" << std::endl;
    } else {
        std::cout << "✗ Failed to create booking: " << errorMsg << std::endl;
    }

    // Booking 2: Bob in Room 201 for 2 nights
    if (manager.createBooking("C002", "201", "2024-06-26", "2024-06-28", errorMsg)) {
        std::cout << "✓ Booking created for Bob Smith" << std::endl;
        std::cout << "  Room: 201 (Deluxe)" << std::endl;
        std::cout << "  Duration: 2024-06-26 to 2024-06-28 (2 nights)" << std::endl;
    } else {
        std::cout << "✗ Failed to create booking: " << errorMsg << std::endl;
    }

    // Booking 3: Carol in Room 301 for 4 nights
    if (manager.createBooking("C003", "301", "2024-06-27", "2024-07-01", errorMsg)) {
        std::cout << "✓ Booking created for Carol Davis" << std::endl;
        std::cout << "  Room: 301 (Suite)" << std::endl;
        std::cout << "  Duration: 2024-06-27 to 2024-07-01 (4 nights)" << std::endl;
    } else {
        std::cout << "✗ Failed to create booking: " << errorMsg << std::endl;
    }

    std::cout << "\nTotal bookings created: " << manager.getBookings().size() << std::endl;

    // ========== STEP 4: Display Room Status (Polymorphic Behavior) ==========
    printHeader("STEP 4: Room Availability Status & Polymorphic Pricing");

    printSeparator();
    std::cout << "Room Details showing polymorphic calculateTargetPrice():" << std::endl;
    printSeparator();

    const auto& allRooms = manager.getRooms();
    for (const auto& room : allRooms) {
        if (room) {
            std::cout << "Room " << room->getRoomNumber();
            std::cout << " (" << (room->getIsAvailable() ? "AVAILABLE" : "BOOKED") << ")";
            std::cout << " - Base: $" << room->getBasePrice() << "/night";
            std::cout << " | Target Price: $" << room->calculateTargetPrice() << std::endl;
        }
    }

    // ========== STEP 5: Test Room Availability Management ==========
    printHeader("STEP 5: Room Availability Management");

    // Try to mark a booked room as available (should fail)
    if (manager.setRoomAvailability("101", true, errorMsg)) {
        std::cout << "✓ Room 101 marked as available" << std::endl;
    } else {
        std::cout << "✗ Cannot mark Room 101 available (it has active booking): " << errorMsg << std::endl;
    }

    // Mark Room 102 as unavailable for maintenance
    if (manager.setRoomAvailability("102", false, errorMsg)) {
        std::cout << "✓ Room 102 marked as unavailable (maintenance)" << std::endl;
    } else {
        std::cout << "✗ Failed to update Room 102 availability: " << errorMsg << std::endl;
    }

    // ========== STEP 6: Create Invoices with Tax ==========
    printHeader("STEP 6: Invoice Generation with Tax Calculation");

    const double taxRate = 0.1;  // 10% tax

    // Find all bookings and create corresponding invoices
    const auto& bookings = manager.getBookings();
    int invoiceCount = 0;

    for (size_t i = 0; i < bookings.size(); ++i) {
        if (bookings[i]) {
            std::string invoiceId = "INV" + std::to_string(i + 1);
            std::string bookingId = "BOOKING_" + std::to_string(i + 1);
            
            // Calculate days between check-in and check-out
            int nights = bookings[i]->getCheckOutDate().daysTo(bookings[i]->getCheckInDate()) * -1;
            
            auto invoice = manager.createInvoice(invoiceId, bookings[i]->getBookingId(), nights, taxRate, errorMsg);
            if (invoice) {
                invoiceCount++;
                auto customer = bookings[i]->getCustomer();
                auto room = bookings[i]->getRoom();
                
                std::cout << "✓ Invoice " << invoiceId << " created" << std::endl;
                if (customer) std::cout << "  Customer: " << customer->getName() << std::endl;
                if (room) {
                    double subtotal = room->getBasePrice() * nights;
                    double tax = subtotal * taxRate;
                    std::cout << "  Room: " << room->getRoomNumber() << " for " << nights << " nights" << std::endl;
                    std::cout << "  Subtotal: $" << subtotal << " + Tax (10%): $" << tax;
                    std::cout << " = Total: $" << invoice->getTotalAmount() << std::endl;
                }
            } else {
                std::cout << "✗ Failed to create invoice " << invoiceId << ": " << errorMsg << std::endl;
            }
        }
    }

    // ========== STEP 7: Display System Summary ==========
    printHeader("STEP 7: System Summary & Statistics");

    printSeparator();
    std::cout << "Hotel Statistics:" << std::endl;
    printSeparator();
    std::cout << "Total Rooms: " << manager.getRooms().size() << std::endl;
    std::cout << "Available Rooms: " << manager.getAvailableRooms().size() << std::endl;
    std::cout << "Booked Rooms: " << (manager.getRooms().size() - manager.getAvailableRooms().size()) << std::endl;
    std::cout << "Total Customers: " << manager.getCustomers().size() << std::endl;
    std::cout << "Total Bookings: " << manager.getBookings().size() << std::endl;
    std::cout << "Total Invoices Created: " << invoiceCount << std::endl;

    // ========== STEP 8: Demonstrate Data Persistence ==========
    printHeader("STEP 8: Data Persistence (Singleton Pattern)");

    DataManager& dataManager = DataManager::getInstance();
    if (dataManager.saveAll("hotel_data")) {
        std::cout << "✓ System data saved successfully to 'hotel_data'" << std::endl;
    } else {
        std::cout << "✗ Failed to save system data" << std::endl;
    }

    if (dataManager.loadAll("hotel_data")) {
        std::cout << "✓ System data loaded successfully from 'hotel_data'" << std::endl;
    } else {
        std::cout << "✗ Failed to load system data" << std::endl;
    }

    // ========== STEP 9: Demonstrate Object Queries ==========
    printHeader("STEP 9: Object Search & Retrieval");

    auto foundCustomer = manager.findCustomerById("C001");
    if (foundCustomer) {
        std::cout << "✓ Found customer: " << foundCustomer->getName() << " (" << foundCustomer->getCustomerId() << ")" << std::endl;
    }

    auto foundRoom = manager.findRoomByNumber("201");
    if (foundRoom) {
        std::cout << "✓ Found room: #" << foundRoom->getRoomNumber() << " - Base Price: $" << foundRoom->getBasePrice() << std::endl;
    }

    printHeader("DEMO COMPLETED SUCCESSFULLY");
    std::cout << "All OOP principles demonstrated:" << std::endl;
    std::cout << "  • Encapsulation: Private attributes with public accessors" << std::endl;
    std::cout << "  • Inheritance: Room inheritance hierarchy (Standard, Deluxe, Suite)" << std::endl;
    std::cout << "  • Polymorphism: Virtual calculateTargetPrice() method" << std::endl;
    std::cout << "  • Abstraction: HotelManager abstracts business logic" << std::endl;
    std::cout << "  • Factory Pattern: RoomFactory for object creation" << std::endl;
    std::cout << "  • Singleton Pattern: DataManager for persistence" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    return 0;
}
