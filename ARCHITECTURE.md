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
main.cpp
  │
  ├─► DataManager::getInstance().loadAll()   // load persisted data on startup
  │
  └─► MainWindow                              // user interaction (View)
        │
        └─► HotelManager                      // business logic (Controller)
              │
              ├─► Models                      // Room, Customer, Booking, Invoice
              │
              └─► DataManager                 // save/load (Persistence)
```

`main.cpp` stays thin: it initializes Qt, loads data, shows the window, and runs the event loop. It does not contain business rules.

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
virtual double calculateRent(int days) const = 0;
```

Each subclass overrides it with its own pricing rule:

| Class | Formula |
|---|---|
| `StandardRoom` | `baseRate × days` |
| `DeluxeRoom` | `baseRate × days × 1.2` |
| `SuiteRoom` | `baseRate × days × 1.5` |

Controller and view code can call `calculateRent()` on any `Room` without knowing the concrete type.

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

### Factory Pattern — `HotelManager::createRoom()`

```cpp
static std::shared_ptr<Room> createRoom(RoomKind kind, int roomNumber, double baseRate = 0.0);
```

**Why:** GUI and controller code request a room by kind (`Standard`, `Deluxe`, `Suite`) instead of constructing concrete classes directly.

**Benefits:**
- Callers depend on the `Room` abstraction, not specific subclasses.
- Adding a new room type requires updating only the factory switch — not every caller.
- Satisfies the lab requirement for at least one design pattern.

Default base rates are applied inside the factory when `baseRate` is not provided (100 / 200 / 350).

### Singleton Pattern — `DataManager`

```cpp
static DataManager& getInstance();
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

## View Layer (Qt6)

| File | Role |
|---|---|
| `MainWindow.ui` | Visual layout designed in Qt Designer |
| `MainWindow.h` | Widget class declaration (`Q_OBJECT`, signals/slots) |
| `MainWindow.cpp` | Behavior — connects UI events to controller logic |

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

The current scaffold provides compilable stubs. Expected next steps by layer:

- **`database/`** — implement `loadAll()` / `saveAll()` with JSON, CSV, or SQLite
- **`controllers/`** — booking validation, availability checks, invoice generation
- **`views/`** — tabbed UI for rooms, customers, bookings; wire signals to `HotelManager`
- **`main.cpp`** — call `saveAll()` on application exit
