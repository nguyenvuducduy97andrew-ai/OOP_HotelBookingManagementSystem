# Architecture

This document describes the structure and design decisions for the **Hotel Booking Management System** — a C++17 / Qt6 desktop application built with an MVC layout and a dedicated persistence layer.

## Directory Layout

```text
├── CMakeLists.txt
├── ARCHITECTURE.md
├── README.md
└── src/
    ├── main.cpp                 # Application entry point
    ├── models/                  # Domain entities (data + OOP hierarchy)
    ├── controllers/             # Business logic
    ├── database/                # Load/save persistence
    └── views/                   # Qt GUI (Widget + .ui)
```

Each layer has a single responsibility so team members can work in parallel with minimal merge conflicts.

| Layer | Responsibility |
|---|---|
| `models/` | What the system manages — rooms, customers, bookings, invoices |
| `controllers/` | How the system behaves — create rooms, manage collections, enforce rules |
| `database/` | Where data lives between sessions — JSON, CSV, or SQLite |
| `views/` | How the user interacts — Qt widgets, layouts, signals/slots |

## High-Level Data Flow

```text
main.cpp (Demo)
  │
  ├─► HotelManager::registerRoom()        // add rooms with validation
  ├─► HotelManager::registerCustomer()    // add customers with validation
  ├─► HotelManager::createBooking()       // create bookings with date/availability checks
  ├─► Room::calculateTargetPrice()        // polymorphic pricing
  ├─► HotelManager::setRoomAvailability() // manage room status
  ├─► HotelManager::createInvoice()       // generate invoices with tax
  ├─► HotelManager::find*()               // query system state
  │
  └─► DataManager::getInstance()          // persist data to disk
        │
        └─► Models (Room, Customer, Booking, Invoice)
```

**main.cpp** is now a comprehensive demo program that:
1. Registers rooms of different types (Standard, Deluxe, Suite)
2. Registers customers
3. Creates bookings with full validation
4. Demonstrates polymorphic room pricing
5. Shows room availability management
6. Generates invoices with tax calculation
7. Performs object queries and retrieval
8. Persists all data using DataManager

The demo showcases all four OOP principles, design patterns, and the complete system workflow.

## OOP Design (Models)

The model layer demonstrates all four OOP pillars required by the lab guidelines.

### Encapsulation

All model classes keep their fields `private` and expose behavior through public getters and setters. External code cannot modify internal state directly (e.g. a room's availability or a customer's phone number).

### Inheritance

```text
Room (abstract)
 ├── StandardRoom
 ├── DeluxeRoom
 └── SuiteRoom
```

`StandardRoom`, `DeluxeRoom`, and `SuiteRoom` inherit common attributes (room number, type, base rate, availability) from `Room` and only override what differs — rent calculation.

### Polymorphism

`Room` declares a pure virtual method:

```cpp
virtual double calculateTargetPrice() = 0;
```

Each subclass overrides it with its own pricing rule:

| Class | Formula |
|---|---|
| `StandardRoom` | `baseRate` (1.0x) |
| `DeluxeRoom` | `baseRate × 1.2` (20% premium) |
| `SuiteRoom` | `baseRate × 1.5` (50% premium) |

Controller and view code can call `calculateTargetPrice()` on any `Room` without knowing the concrete type.

### Abstraction

`Room` is an abstract base class — it cannot be instantiated directly. Callers work with the `Room` interface; concrete types are created through the factory (see below).

### Entity Relationships

```text
Customer ──┐
           ├── Booking ── Invoice
Room ──────┘
```

- **`Customer`** — identity and contact info (ID, name, phone). Kept separate so one customer can hold multiple bookings.
- **`Booking`** — links a customer to a room with check-in and check-out dates. Represents the reservation itself.
- **`Invoice`** — wraps a booking and computes subtotal, tax, and total. Separates billing from reservation logic.

### Header / Source Split

Every model class uses a `.h` (declaration) and `.cpp` (implementation) pair:

- Faster incremental builds — changing a `.cpp` does not force recompilation of every file that includes the header.
- Cleaner interfaces — headers expose only what other modules need.
- Matches standard C++ project conventions expected in grading.

## Design Patterns

### Factory Pattern — `RoomFactory`

```cpp
static std::shared_ptr<Room> createRoom(RoomType type, std::string num, double basePrice);
```

**Why:** The system decouples room creation logic from the controller. Callers request a room by type enum (`Standard`, `Deluxe`, `Suite`) rather than constructing concrete classes directly.

**Benefits:**
- Callers depend on the `Room` abstraction, not specific subclasses.
- Adding a new room type requires updating only the factory — not every caller.
- Encapsulates object creation logic in one place.

### Validation-First Controller Pattern — `HotelManager`

```cpp
bool registerRoom(RoomType kind, const std::string& roomNumber, double baseRate, std::string& errorMessage);
bool registerCustomer(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
bool createBooking(const std::string& customerId, const std::string& roomNumber, const std::string& checkInDate, const std::string& checkOutDate, std::string& errorMessage);
bool setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage);
```

**Why:** All public use-case methods perform input validation before modifying state. Errors are returned as strings rather than exceptions, making the system more resilient and testable.

**Key Methods:**
- `registerRoom()`, `registerCustomer()` — validate input, create objects, add to collections
- `createBooking()` — validates customer exists, room is available, dates are valid, then marks room unavailable
- `setRoomAvailability()` — prevents marking a booked room as available
- `createInvoice()` — computes billing based on base price, duration, and tax rate

### Singleton Pattern — `DataManager`

```cpp
static DataManager& getInstance();
bool saveAll(const std::string& prefix);
bool loadAll(const std::string& prefix);
```

**Why:** Persistence should have a single access point for loading and saving data, regardless of whether the backend is JSON, CSV, or SQLite.

**Benefits:**
- One shared instance avoids duplicate file handles or conflicting writes.
- Copy and assignment are deleted to prevent accidental second instances.
- The rest of the application calls `DataManager::getInstance()` without managing its lifetime.

## Memory Management

The project uses `std::shared_ptr` for heap-allocated model objects stored in `HotelManager`:

```cpp
std::vector<std::shared_ptr<Room>> rooms;
std::vector<std::shared_ptr<Customer>> customers;
std::vector<std::shared_ptr<Booking>> bookings;
```

**Why `shared_ptr` and not raw pointers:**
- `Room` is polymorphic — pointer semantics are required to store subclasses through a base pointer.
- A `Booking` references the same `Customer` and `Room` objects held by the manager — shared ownership prevents use-after-free and double-delete.
- No manual `delete` calls — satisfies the lab memory-management requirement cleanly.

## HotelManager API

### Core Methods

**Room Management:**
```cpp
bool registerRoom(RoomType kind, const std::string& roomNumber, double baseRate, std::string& errorMessage);
bool setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage);
```

**Customer Management:**
```cpp
bool registerCustomer(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
```

**Booking Management:**
```cpp
bool createBooking(
    const std::string& customerId,
    const std::string& roomNumber,
    const std::string& checkInDate,      // ISO format: "YYYY-MM-DD"
    const std::string& checkOutDate,     // ISO format: "YYYY-MM-DD"
    std::string& errorMessage
);
```

**Invoice Generation:**
```cpp
std::shared_ptr<Invoice> createInvoice(
    const std::string& invoiceId,
    const std::string& bookingId,
    int days,                             // duration in nights
    double taxRate,                       // 0.1 = 10%
    std::string& errorMessage
) const;
```

### Query Methods

```cpp
const std::vector<std::shared_ptr<Room>>& getRooms() const;
const std::vector<std::shared_ptr<Customer>>& getCustomers() const;
const std::vector<std::shared_ptr<Booking>>& getBookings() const;

std::shared_ptr<Room> findRoomByNumber(const std::string& roomNumber) const;
std::shared_ptr<Customer> findCustomerById(const std::string& customerId) const;
std::shared_ptr<Booking> findBookingById(const std::string& bookingId) const;

std::vector<std::shared_ptr<Room>> getAvailableRooms() const;
```

### Validation & Error Handling

All public methods validate inputs and return errors via reference parameter:

- Invalid room numbers (empty, non-alphanumeric)
- Duplicate room numbers or customer IDs
- Invalid base rates (≤ 0)
- Invalid date formats (must be ISO "YYYY-MM-DD")
- Invalid date ranges (checkout ≤ checkin)
- Room already booked
- Customer or room not found

This validation-first approach makes the system robust and testable.

## View Layer (Qt6)

The view layer is currently in development with the following structure:

| File | Role | Status |
|---|---|---|
| `MainWindow.ui` | Visual layout designed in Qt Designer | Scaffolding |
| `MainWindow.h` | Widget class declaration (`Q_OBJECT`, signals/slots) | Scaffolding |
| `MainWindow.cpp` | Behavior — connects UI events to controller logic | Scaffolding |

**Current State:** The system runs as a comprehensive console demo in `main.cpp` that demonstrates all functionality. This allows validation of business logic before integrating with the Qt GUI.

**Next Step:** Wire `MainWindow` signals/slots to `HotelManager` methods to provide interactive GUI control.

The `.ui` file is compiled automatically by CMake (`AUTOUIC`). The generated `ui_MainWindow.h` is included in `MainWindow.cpp`, not edited by hand.

**Why Widget + `.ui`:** Matches the lab requirement for Qt6 QWidget, UI files, and the signal/slot mechanism. Layout changes stay in Designer; logic stays in C++.

## Build System (CMake)

Key settings in `CMakeLists.txt`:

| Setting | Purpose |
|---|---|
| `CMAKE_CXX_STANDARD 17` | Smart pointers, `enum class`, modern C++ features |
| `AUTOMOC` | Auto-generates MOC code for Qt signals/slots |
| `AUTOUIC` | Compiles `.ui` files into C++ headers |
| `find_package(Qt6 Widgets)` | Links the Qt6 widget library |
| `target_include_directories` | Allows short includes like `#include "Room.h"` across layers |

**Adding a new class:** register both the `.h` and `.cpp` in the `add_executable(...)` block under the correct layer group, then reconfigure the build.

## Layer Boundaries (Conventions)

| From → To | Allowed? | Notes |
|---|---|---|
| `views/` → `controllers/` | Yes | UI delegates actions to `HotelManager` |
| `controllers/` → `models/` | Yes | Business logic operates on entities |
| `controllers/` → `database/` | Yes | Save/load through `DataManager` |
| `models/` → `controllers/` | No | Models must not depend on business logic |
| `models/` → `views/` | No | Models must not depend on Qt GUI |
| `database/` → `models/` | Yes (future) | Persistence reads/writes model data |

Keeping these boundaries strict makes the codebase easier to test, extend, and grade.

## Future Work

The current implementation provides:

✅ **Complete** — OOP design with all four pillars (encapsulation, inheritance, polymorphism, abstraction)
✅ **Complete** — Factory Pattern (RoomFactory) and Singleton Pattern (DataManager)
✅ **Complete** — Comprehensive validation and error handling in HotelManager
✅ **Complete** — Polymorphic room pricing (calculateTargetPrice)
✅ **Complete** — Invoice generation with tax calculation
✅ **Complete** — Working demo in main.cpp

**Next Steps:**

- **`database/`** — implement `loadAll()` / `saveAll()` persistence layer (JSON, CSV, or SQLite)
- **`views/`** — integrate MainWindow with HotelManager (tabbed UI for rooms, customers, bookings, invoices)
- **Qt Integration** — connect GUI signals/slots to HotelManager methods for interactive use
- **Error Display** — show validation errors in UI dialogs rather than console output
- **Data Loading** — call `DataManager::getInstance().loadAll()` on MainWindow initialization
