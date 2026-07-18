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
- **Availability Tracking**: Track from booking data. `isAvailable` marks maintenance/inspection status only — independent of whether a guest is currently staying in the room
- **Room Factory Pattern**: Safe room creation with validation
- **Archive-first cleanup**: Hard delete is restricted and intended only for admin/data-cleanup. Archiving rooms or customers is the preferred way to hide old entities while preserving booking and invoice history.

### Customer Management
- **Customer Records**: Store customer ID, name, and phone number
- **Unique ID Enforcement**: Prevent duplicate customer entries
- **Multi-booking Support**: One customer can hold multiple reservations, queryable via `getBookingsForCustomer()`

### Booking System
- **Reservation Management**: Link customers to rooms with check-in/check-out dates
- **Booking Validation**: Prevent invalid date ranges and double-booking (overlap checks skip cancelled bookings)
- **Derived Status**: Upcoming/Active/Completed states are computed from dates at read time, never stored
- **Cancellation**: `cancelBooking()` soft-cancels an upcoming reservation and cascades a void (not delete) to any attached invoice, so front-desk staff can cancel in one action without tracing invoices manually
- **Occupancy Queries**: `getRoomsByOccupancy()`, `getTodayCheckIns()`, `getTodayCheckOuts()` derive real-time occupancy from bookings.
- **Automatic ID Generation**: Unique booking identifiers
- **Available Room Filtering**: Query empty rooms for specific periods

### Invoice & Billing
- **Invoice Lifecycle**: Create, lookup, list, and delete invoices through HotelManager
- **Flexible Timing**: Invoices can be generated at booking time (pay-first) or at checkout — not gated to a single booking state
- **Billing Separation**: Invoices remain tied to bookings while preserving their own payment date and tax configuration
- **Cancellation Handling**: If a booking with an existing invoice is cancelled, the invoice is voided (marked cancelled, not deleted) automatically, preserving an audit trail while excluding it from revenue totals

### Data Persistence
- **SQLite-backed Persistence**: Load and save hotel state through DataManager::loadAll() and DataManager::saveAll()
- **Automatic Database Setup**: The data layer creates the required tables on first use
- **Core Domain Recovery**: Customer, room, and booking data are restored from the database on startup
- **Strict Persistence Integrity**: `saveAll()` refuses to persist invalid object graphs and rolls back on broken booking or invoice references, instead of silently dropping bad child records.

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

## 🖥️ UI Layer
The project now includes a complete Qt desktop interface in `src/views/`, with separated view classes that keep the user experience distinct from business logic.

Key UI components:
- `MainWindow` — application shell and navigation
- `DashboardWidget` — summary cards and room/booking charts
- `ReservationsPageWidget` — booking list, filtering, and appointment actions
- `RoomPageWidget` — room listing, details, add/edit/delete workflows
- `RoomStatusPageWidget` — room status overview and maintenance controls
- `ReservationDialog`, `RoomDialog`, `InvoiceDialog` — modal forms for creating and editing entities
- `CustomConfirmDialog`, `CustomSuccessDialog` — reusable confirmation and success feedback dialogs

This UI layer operates on English interface text while preserving existing controller and model behavior.

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
- **Repository Pattern** (HotelManager) — Centralized collection access, including derived-state queries (booking status, room occupancy) computed on read rather than stored redundantly

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

The project includes a Qt desktop application entry point in `main.cpp` with the following flow:
1. Initialize the `QApplication` runtime
2. Construct `HotelManager` as the core controller state
3. Load persisted hotel data from SQLite via `DataManager::loadAll()`
4. Inject `HotelManager` into `MainWindow`
5. Display the user interface and process user actions in the Qt event loop
6. Persist in-memory changes before shutdown via `DataManager::saveAll()`

Run the application to see the UI in action:
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
