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
- **Tax Calculation**: Configurable tax rates applied to subtotals
- **Itemized Invoices**: Separate billing from reservation logic
- **Financial Tracking**: Total cost, subtotal, and tax breakdowns

### Data Persistence
- **Automatic Save/Load**: Persist all data between sessions
- **Multiple Backends**: Support for JSON, CSV, or SQLite databases
- **DataManager Singleton**: Centralized data access layer

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
  ├─► DataManager::getInstance().loadAll()    ← Load data on startup
  └─► MainWindow / Console Demo                ← User interactions
        └─► HotelManager                       ← Business logic
              ├─► Models (Room, Customer, etc.) ← Domain objects
              └─► DataManager                  ← Persistence
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
| `HotelManager` | Controller managing rooms, customers, bookings |
| `DataManager` | Singleton for data persistence |
| `RoomFactory` | Factory for creating room instances |
| `MainWindow` | Qt GUI (future expansion) |

---

## 📝 Demo

The project includes a comprehensive console demo (`main.cpp`) showcasing:
1. Adding rooms (Standard, Deluxe, Suite)
2. Registering customers
3. Creating bookings
4. Generating invoices
5. Calculating totals with tax
6. Displaying reports

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

---

## 📄 License

Educational project for OOP coursework.
