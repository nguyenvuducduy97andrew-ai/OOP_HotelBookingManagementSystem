# Hotel Booking Management System

**Object-Oriented Programming Project** — Group 10, Class 25C10, University of Science, Vietnam National University Ho Chi Minh City

---

## 📋 Project Overview

A **C++17 desktop application** for managing hotel bookings, customers, rooms, and invoices. Built with Qt6 and designed with clean MVC (Model-View-Controller) architecture and SOLID principles.

The system demonstrates core object-oriented programming concepts:
- **Encapsulation** — Private data with public interfaces
- **Inheritance** — Room hierarchy with base and derived classes
- **Polymorphism** — Virtual methods for rent calculation
- **Abstraction** — Abstract base classes and interfaces

---

## ✨ Key Features

### Room Management
- **Three Room Types**: Standard, Deluxe, and Suite rooms with distinct pricing models
- **Polymorphic Pricing**: Each room type calculates rent differently
  - **Standard**: `baseRate × days`
  - **Deluxe**: `baseRate × days × 1.2` (20% premium)
  - **Suite**: `baseRate × days × 1.5` (50% premium)
- **Availability Tracking**: Monitor room occupancy status
- **Room Factory Pattern**: Safe room creation with validation

### Customer Management
- **Customer Records**: Store customer ID, name, and phone number
- **Unique ID Enforcement**: Prevent duplicate customer entries
- **Multi-booking Support**: One customer can hold multiple reservations

### Booking System
- **Reservation Management**: Link customers to rooms with check-in/check-out dates
- **Booking Validation**: Prevent invalid date ranges and double-booking
- **Automatic ID Generation**: Unique booking identifiers
- **Available Room Filtering**: Query empty rooms for specific periods

### Invoice & Billing
- **Invoice Lifecycle**: Create, lookup, list, and delete invoices through HotelManager
- **Billing Separation**: Invoices remain tied to bookings while preserving their own payment date and tax configuration

### Data Persistence
- **SQLite-backed Persistence**: Load and save hotel state through DataManager::loadAll() and DataManager::saveAll()
- **Automatic Database Setup**: The data layer creates the required tables on first use
- **Core Domain Recovery**: Customer, room, and booking data are restored from the database on startup

---

## 🏗️ Architecture

### Directory Structure
```
src/
├── main.cpp              # Application entry point
├── models/               # Domain entities (Room, Customer, Booking, Invoice)
├── controllers/          # Business logic (HotelManager)
├── database/             # Persistence layer (DataManager)
└── views/                # Qt GUI components (MainWindow)
```

### MVC Layers

| Layer | Responsibility |
|---|---|
| **Models** | Room hierarchy, Customer, Booking, Invoice — core domain objects |
| **Controller** | HotelManager — orchestrates operations, enforces business rules |
| **Database** | DataManager — loads and saves data to persistent storage |
| **Views** | Qt widgets and dialogs — user interface |

### Data Flow
```
main.cpp
  ├─► DataManager::getInstance().loadAll()    ← Load persisted data on startup
  └─► MainWindow / Console Demo                ← User interactions
        └─► HotelManager                       ← Business logic
              ├─► Models (Room, Customer, Booking, Invoice)
              └─► DataManager::saveAll()        ← Persist state to SQLite
```

### Memory Management Strategy
- **Ownership Model**: HotelManager owns all domain objects via `std::shared_ptr`
- **Reference Integrity**: Business objects use `std::weak_ptr` to reference each other safely
- **No Memory Leaks**: Automatic cleanup when objects are deleted from collections
- **Safety**: weak_ptr::lock() prevents dangling pointer access

### Data Persistence
- **SQLite Database**: Automatic schema creation and management
- **Singleton DataManager**: Central controller for all load/save operations
- **Automatic Recovery**: Customer, room, and booking data restore on startup
- **Safe Shutdown**: All in-memory state persists to database via DataManager::saveAll()

---

## 🎯 OOP Design Highlights

### Inheritance Hierarchy
```cpp
Room (abstract base class)
├── StandardRoom
├── DeluxeRoom
└── SuiteRoom
```

Each subclass inherits common attributes from `Room` and overrides `calculateTargetPrice()` with unique pricing logic.

### Entity Relationships
```
Customer
    ├─ Can create multiple Bookings
    
Room
    ├─ Subject of multiple Bookings
    ├─ Polymorphic pricing (Standard, Deluxe, Suite)
    
Booking
    ├─ Links Customer + Room
    ├─ Tracks availability conflicts
    
Invoice
    ├─ Associated with one Booking
    ├─ Calculates cost with tax
    ├─ Persists payment metadata
```

- **One-to-Many**: One customer can have multiple bookings
- **Booking**: Junction entity linking customers to rooms with date ranges
- **Invoice**: Wraps booking for billing purposes with tax and payment tracking

### Design Patterns Used
- **Factory Pattern** (RoomFactory) — Encapsulate room creation
- **Singleton Pattern** (DataManager) — Single instance for data management
- **Strategy Pattern** (Room subclasses) — Different pricing strategies
- **Repository Pattern** (HotelManager) — Centralized collection access

### Business Rules & Validation
- **Room Number Uniqueness**: No two rooms can have the same number
- **Customer ID Uniqueness**: Each customer has a unique identifier
- **Date Validation**: Check-in must be before check-out; dates must be valid ISO format (YYYY-MM-DD)
- **Availability Conflict Prevention**: Rooms cannot be double-booked for overlapping periods
- **Tax Rate Validation**: Must be a valid percentage (0.0 to 1.0)
- **Existence Verification**: Referenced entities (customer, room, booking) must exist before linking
- **Error Reporting**: All validation failures return detailed error messages to caller

---

## � Data Persistence

The SQLite database automatically stores and manages:

| Table | Content |
|---|---|
| **rooms** | Room number, type (Standard/Deluxe/Suite), base price, availability status |
| **customers** | Customer ID, name, phone number |
| **bookings** | Booking ID, customer reference, room reference, check-in/check-out dates |
| **invoices** | Invoice ID, booking reference, tax rate, nights stayed, payment date |

**Persistence Workflow:**
- **Startup**: `DataManager::loadAll()` restores all entities from database into HotelManager
- **Runtime**: In-memory collections reflect current state
- **Shutdown**: `DataManager::saveAll()` persists current state back to SQLite
- **Schema Initialization**: Database tables are automatically created on first use if they don't exist

---

## �️ Technology Stack

- **Language**: C++17
- **Framework**: Qt6 (Core, Widgets, SQL)
- **Build System**: CMake 3.16+
- **Architecture**: MVC with Persistence Layer
- **Compiler**: MSVC (Visual Studio)

---

## 🚀 Getting Started

### Prerequisites
- CMake 3.16 or later
- Qt6 (Core, Widgets, SQL components)
- C++17 compatible compiler (MSVC, GCC, or Clang)

### Build & Run
```bash
mkdir build
cd build
cmake ..
cmake --build .
./HotelBookingManagement
```

---

## 📊 Main Classes

| Class | Purpose |
|---|---|
| `Room` | Abstract base for all room types |
| `StandardRoom`, `DeluxeRoom`, `SuiteRoom` | Concrete room implementations |
| `Customer` | Represents a guest |
| `Booking` | Links customer to room with dates |
| `Invoice` | Billing information for a booking |
| `HotelManager` | Controller managing rooms, customers, bookings, and invoice operations |
| `DataManager` | Singleton for SQLite-backed data persistence via loadAll()/saveAll() |
| `RoomFactory` | Factory for creating room instances |
| `MainWindow` | Qt GUI (future expansion) |

---

## 📝 Demo

The project includes a comprehensive console demo (`main.cpp`) showcasing:
1. Adding rooms (Standard, Deluxe, Suite)
2. Registering customers
3. Creating bookings
4. Generating invoices
5. Listing and deleting invoices when needed
6. Persisting and reloading state with DataManager::saveAll()/loadAll()

Run the demo to see the system in action:
```bash
./HotelBookingManagement
```

---

## 📚 Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** — Detailed design decisions and data flow
- **CMakeLists.txt** — Build configuration and dependency management

---

## 👥 Contributors

Group 10, Class 25C10
University of Science, Vietnam National University Ho Chi Minh City

| **Student ID** |        **Name**       | **Assigned Tasks**                                                                                              | **% Completed** |
|:--------------:|:---------------------:|-----------------------------------------------------------------------------------------------------------------|-----------------|
|    25127070    |     Le Quang Khai     | Base rooms class design and implementations                                                                     |                 |
|    25127190    |   Nguyen Vu Duc Duy   | Backend system architecture design. Hotel management class implementation. Memory safety handling.              |                 |
|    25127234    | Nguyen Thi Thao Suong | Customer class implementation.                                                                                  |                 |
|    25127389    |    Nguyen Minh Khoi   | Booking and Invoice class implementation. Data management class design and implementation. User-flow designing. |                 |
|    25127464    |    Nguyen Kim Phuc    | User-Interface research                                                                                         |                 |                                                                                       |                 |
## 📄 License

Educational project for OOP coursework.
