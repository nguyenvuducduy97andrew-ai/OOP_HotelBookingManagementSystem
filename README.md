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
Customer ──┐
           ├──► Booking ──► Invoice
Room ──────┘
```

- **One-to-Many**: One customer can have multiple bookings
- **Booking**: Junction entity linking customers to rooms
- **Invoice**: Wraps booking for billing purposes

### Design Patterns Used
- **Factory Pattern** (RoomFactory) — Encapsulate room creation
- **Singleton Pattern** (DataManager) — Single instance for data management
- **Strategy Pattern** (Room subclasses) — Different pricing strategies
- **Repository Pattern** (HotelManager) — Centralized collection access

---

## 🛠️ Technology Stack

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
| `Student ID` |          `Name`         |
|:----------:|:---------------------:|
|  25127070  |     Le Quang Khai     |
|  25127190  |   Nguyen Vu Duc Duy   |
|  25127234  | Nguyen Thi Thao Suong |
|  25127389  |    Nguyen Minh Khoi   |
|  25127464  |    Nguyen Kim Phuc    |
---

## 📄 License

Educational project for OOP coursework.
