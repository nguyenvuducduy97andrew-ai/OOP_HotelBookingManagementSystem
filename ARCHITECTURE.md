# Architecture

This document describes the structure and design decisions for the **Hotel Booking Management System** — a C++17 / Qt6 desktop application built with an MVC layout and a dedicated SQLite persistence layer.

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
    └── views/                   # Qt GUI (widgets, dialogs, and .ui files)
```

Each layer has a single responsibility so team members can work in parallel with minimal merge conflicts.

| Layer | Responsibility |
|---|---|
| `models/` | What the system manages — rooms, customers, bookings, invoices, and booking/room metadata helpers |
| `controllers/` | How the system behaves — create rooms, manage collections, enforce rules |
| `database/` | Where data lives between sessions — SQLite |
| `views/` | How the user interacts — Qt widgets, layouts, signals/slots, and dialogs |

### View Layer / Qt UI
The `views/` folder contains the application’s Qt desktop interface. It includes the main window, dashboard page, reservation management page, room management page, room status overview, and supporting dialogs such as reservation, room, invoice, confirmation, and success dialogs.

The UI layer is intentionally separated from controller logic: view classes emit user events and invoke `HotelManager` operations, while business rules and persistence remain inside the controller and model layers.

## High-Level Data Flow

```text
main.cpp
  ├─► QApplication app(argc, argv)           // start Qt runtime
  ├─► HotelManager hotelManager               // create core controller state
  ├─► DataManager::getInstance().loadAll()    // load persisted hotel state from SQLite
  ├─► MainWindow mainWindow(&hotelManager)     // inject controller into Qt UI
  ├─► mainWindow.show()                       // display the desktop interface
  ├─► app.exec()                              // Qt event loop processes user actions
  │     ├─► views/                           // UI widgets send commands to HotelManager
  │     └─► HotelManager                      // business logic updates models
  └─► DataManager::getInstance().saveAll()    // persist changes at shutdown
```

This startup and shutdown flow reflects the current Qt desktop app architecture: persisted data is loaded before the main window is shown, the view layer operates against `HotelManager`, and changes are saved when the application exits. The SQLite database file is stored in the project-level `data/` directory as `data/hotel_data.db`, keeping runtime state separate from the source tree and build artifacts.

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

`StandardRoom`, `DeluxeRoom`, and `SuiteRoom` inherit common attributes (room number, base rate, availability) from `Room` and override the room-specific behavior that differs — target price, room type name, and subtype fee accessors used by the UI.

### Polymorphism

`Room` declares pure virtual methods:

```cpp
virtual double calculateTargetPrice() const = 0;
virtual std::string getRoomTypeName() const = 0;
virtual double getExtraFeeAmount() const = 0;
```

Each subclass overrides them with its own pricing rule and metadata:

| Class | Formula |
|---|---|
| `StandardRoom` | `baseRate` |
| `DeluxeRoom` | `baseRate + miniBarFee` |
| `SuiteRoom` | `baseRate + premiumServiceFee` |

Controller and view code can call `calculateTargetPrice()` on any `Room` without knowing the concrete type.

### Abstraction

`Room` is an abstract base class — it cannot be instantiated directly. Callers work with the `Room` interface; concrete types are created through the factory (see below).

### Entity Relationships

```text
HotelManager (owns via shared_ptr)
  ├─ Customer ─ ─ ─ ─ ┐
  ├─ Room ─ ─ ─ ─ ┐   │
  └─ Booking       ├─→ (weak references)
        └─ Invoice ┘
```

- **`Customer`** — identity and contact info (ID, name, phone). Kept separate so one customer can hold multiple bookings.
- **`Booking`** — links a customer to a room with check-in and check-out dates via weak references. Represents the reservation itself. The booking state enum lives alongside `Booking` in `Booking.h`, while `HotelManager::getBookingState()` derives the current state from dates and cancellation flags.
  - Stores `weak_ptr<Customer>` and `weak_ptr<Room>` (non-owning references)
  - Stores a persisted `cancelled` flag (see [Booking Lifecycle & Cancellation](#booking-lifecycle--cancellation) below). All other status (upcoming/active/completed) is derived, never stored.
- **`Invoice`** — wraps a booking and computes subtotal, tax, and total via a weak reference. Separates billing from reservation logic.
  - Stores `weak_ptr<Booking>` (non-owning reference)
  - Stores payment date, tax rate, and nights stayed; validity depends on a linked booking, a positive stay duration, and an ISO payment date.

This design ensures that:
1. HotelManager maintains sole ownership of all objects
2. Deleting a customer, room, or booking from HotelManager invalidates only the weak_ptrs
3. No dangling pointers can occur — lock() returns nullptr if the object was deleted

### Header / Source Split

Every model class uses a `.h` (declaration) and `.cpp` (implementation) pair:

- Faster incremental builds — changing a `.cpp` does not force recompilation of every file that includes the header.
- Cleaner interfaces — headers expose only what other modules need.
- Matches standard C++ project conventions expected in grading.

## Booking Lifecycle & Cancellation

`Booking` status is a mix of **derived** and **stored** state — only what cannot be computed is persisted.

```cpp
enum class BookingState { UPCOMING, ACTIVE, COMPLETED, CANCELLED };
BookingState HotelManager::getBookingState(const Booking& booking) const;
```

- `UPCOMING` / `ACTIVE` / `COMPLETED` are derived at read time from `(checkInDate, checkOutDate, today)`. They are never stored, so there is no risk of a stale status drifting from the actual dates.
- `CANCELLED` is the one fact dates alone can never express — it is a deliberate staff action, so `Booking` stores a persisted `cancelled` flag. `getBookingState()` checks this flag first, before falling back to the date-derived states.

**Cancellation is a soft action, distinct from cleanup deletion:**

```cpp
bool cancelBooking(const std::string& bookingId, std::string& errorMessage);
```

- Only valid when the booking's current state is `UPCOMING` — a stay that has already started or finished cannot be "un-happened."
- Sets `Booking::cancelled = true`. The booking record is kept (not removed) for history/audit purposes.
- The booking cleanup path removes the booking record from the manager collection when an admin/data-cleanup action is chosen; it is separate from `cancelBooking()` and is not used by the normal cancellation flow.

**Double-booking overlap checks must exclude cancelled bookings.** The existing overlap guard (`checkIn < existingCheckOut && existingCheckIn < checkOut`) skips any booking where `isCancelled()` is true — otherwise a cancelled booking would permanently block its original dates from being rebooked.

**Revenue/statistics queries must exclude cancelled invoices.** Any method that sums, lists, or counts invoices as "paid" (dashboard totals, income reports) must filter out invoices where `isCancelled()` is true.



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
bool cancelBooking(const std::string& bookingId, std::string& errorMessage);
bool setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage);
bool createInvoice(const std::string& invoiceId, const std::string& bookingId, double taxRate, int nights, const std::string& paymentDate, std::string& errorMessage);
BookingState getBookingState(const Booking& booking) const;
```

- Hard delete operations exist only for admin/data-cleanup. The recommended mechanism for retiring rooms and customers is archive mode, which preserves booking and invoice history while hiding old entities from normal availability queries.

**Why:** All public use-case methods perform input validation before modifying state. Errors are returned as strings rather than exceptions, making the system more resilient and testable.

**Key Methods:**
- `registerRoom()`, `registerCustomer()` — validate input, create objects, add to collections
- `createBooking()` — validates customer exists, room and dates don't overlap with any *non-cancelled* existing booking for that room, then creates the booking. Does **not** touch `Room::isAvailable` — occupancy is derived from bookings, not stored on the room (see [Room Availability vs. Occupancy](#room-availability-vs-occupancy) below).
- `cancelBooking()` — soft-cancels an `UPCOMING` booking and cascades a void to its invoice, if any (see [Booking Lifecycle & Cancellation](#booking-lifecycle--cancellation))
- `setRoomAvailability()` — toggles maintenance status only; blocks marking a room unavailable while it has an `ACTIVE` booking (a guest currently checked in)
- `createInvoice()` — validates invoice input and attaches the invoice to a booking; only allowed once the booking is `ACTIVE` or `COMPLETED`. Pay-first invoicing on `UPCOMING` bookings is intentionally rejected.
- `getBookingState()` — derives `UPCOMING` / `ACTIVE` / `COMPLETED` from dates, or returns `CANCELLED` if the booking's persisted flag is set


### Singleton Pattern — `DataManager`

```cpp
static DataManager& getInstance();
bool saveAll(const HotelManager& manager, const std::string& dataPath = "hotel_data.db");
bool loadAll(HotelManager& manager, const std::string& dataPath = "hotel_data.db");
```

**Why:** Persistence should have a single access point for loading and saving data through the SQLite backend.

**Benefits:**
- One shared instance avoids duplicate file handles or conflicting writes.
- Copy and assignment are deleted to prevent accidental second instances.
- The rest of the application calls `DataManager::getInstance()` without managing its lifetime.
- The data layer initializes the database schema automatically and restores persisted customers, rooms, and bookings.
- `saveAll()` enforces object graph integrity; it refuses to persist broken bookings or invoices and rolls back the transaction instead of silently dropping invalid child records.

### Non-Owning Reference Pattern — weak_ptr

Model objects use `std::weak_ptr` to store non-owning references to entities:
- `Booking` holds weak references to Customer and Room
- `Invoice` holds a weak reference to Booking

**Why weak_ptr instead of shared_ptr:**
- Avoids circular reference problems and ensures HotelManager has clear ownership
- `weak_ptr::lock()` safely returns `nullptr` if the referenced object was deleted
- Prevents dangling pointer bugs when objects are removed from HotelManager
- Forces callers to check validity before use, improving code safety

**Example:**
```cpp
auto customer = booking->getCustomer();  // Returns shared_ptr (from lock())
if (customer != nullptr) {
    // Safe to use
}
// If HotelManager deleted the customer, lock() would return nullptr
```

## Memory Management

The project uses a **non-owning reference pattern** with `std::weak_ptr` for model relationships, ensuring safe pointer semantics and preventing dangling pointer risks.

### Smart Pointer Strategy

**Ownership:**
- `HotelManager` owns all model objects via `std::shared_ptr` vectors:
  ```cpp
  std::vector<std::shared_ptr<Room>> rooms;
  std::vector<std::shared_ptr<Customer>> customers;
  std::vector<std::shared_ptr<Booking>> bookings;
  ```
- Rooms, Customers, and Bookings are created, stored, and destroyed by HotelManager

**References (Non-owning):**
- `Booking` stores weak references to Customer and Room:
  ```cpp
  std::weak_ptr<Customer> customer;
  std::weak_ptr<Room> room;
  ```
- `Invoice` stores a weak reference to Booking:
  ```cpp
  std::weak_ptr<Booking> booking;
  ```

### Why weak_ptr?

**Problem with Raw Pointers:**
- Raw pointers don't indicate ownership
- If HotelManager deletes a Room but an Invoice still holds a raw pointer, accessing it causes undefined behavior (dangling pointer)
- No way to detect at runtime if a pointer is still valid

**Solution with weak_ptr:**
- `weak_ptr` **does not prevent object deletion** — ownership remains solely with HotelManager
- `weak_ptr::lock()` safely converts to `shared_ptr` and returns `nullptr` if the object was deleted
- Callers can check validity before use
- Eliminates dangling pointer risks

### Usage Pattern

**Setting references:**
```cpp
booking->setCustomer(managerCustomerSharedPtr);  // Pass shared_ptr
booking->setRoom(managerRoomSharedPtr);          // Pass shared_ptr
```

**Getting references:**
```cpp
auto customer = booking->getCustomer();  // Returns shared_ptr (locked)
auto room = booking->getRoom();          // Returns shared_ptr (locked)

if (customer) {
    // Safe to use; customer is still alive
    std::cout << customer->getName();
}
// After this block, shared_ptr auto-releases if no other owners
```

**In Invoice:**
```cpp
auto lockedBooking = invoice->getBooking();  // Lock weak_ptr safely
if (lockedBooking != nullptr && lockedBooking->isValid()) {
    // Booking and its Customer/Room are guaranteed valid in this scope
}
```

### Validation Methods

Both Booking and Invoice provide `isValid()` methods that safely lock all weak_ptrs:

```cpp
// Booking::isValid()
bool Booking::isValid() const {
    auto lockedCustomer = customer.lock();
    auto lockedRoom = room.lock();
    return lockedCustomer != nullptr && lockedRoom != nullptr &&
           checkInDate.isValid() && checkOutDate.isValid() &&
           checkOutDate > checkInDate;
}
```

This ensures all referenced objects still exist before use.

## Room Availability vs. Occupancy

`Room::isAvailable` and "does this room currently have a guest" are **independent facts**, not the same flag wearing two hats:

| Fact | Source of truth | Meaning |
|---|---|---|
| Occupancy (vacant/occupied) | Derived from `Booking` records via `getBookingState()` | Does an `ACTIVE` booking exist for this room today? |
| Availability (`isAvailable`) | Stored on `Room`, toggled via `setRoomAvailability()` | Is the room blocked for maintenance/inspection/cleaning? |

These can combine freely — a room can be vacant *and* under maintenance, or occupied *and* fully available for future booking once the current guest checks out. `createBooking()` never sets `isAvailable`; it only checks for date overlaps against existing non-cancelled bookings. `setRoomAvailability(false, ...)` (putting a room into maintenance) is blocked if the room currently has an `ACTIVE` booking — a room with a guest in it isn't a candidate for cleaning/inspection yet. `getRoomsByOccupancy(bool)` derives strictly from booking state and never reads `isAvailable`.

## HotelManager API


### Core Methods

**Room Management:**
```cpp
bool registerRoom(RoomType kind, const std::string& roomNumber, double baseRate, std::string& errorMessage);
bool setRoomAvailability(const std::string& roomNumber, bool available, std::string& errorMessage);
```
`setRoomAvailability` governs maintenance status only (see [Room Availability vs. Occupancy](#room-availability-vs-occupancy)). Setting `available = false` is rejected if the room currently has an `ACTIVE` booking.

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
bool cancelBooking(const std::string& bookingId, std::string& errorMessage);
```

**Invoice Generation:**
```cpp
bool createInvoice(
  const std::string& invoiceId,
  const std::string& bookingId,
  double taxRate,
  int nights,
  const std::string& paymentDate,
  std::string& errorMessage
);
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
std::vector<std::shared_ptr<Room>> getRoomsByOccupancy(bool occupied) const;   // derived from bookings, not isAvailable
std::vector<std::shared_ptr<Booking>> getTodayCheckIns() const;
std::vector<std::shared_ptr<Booking>> getTodayCheckOuts() const;
std::vector<std::shared_ptr<Booking>> getBookingsForCustomer(const std::string& customerId) const;
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

This validation-first approach makes the system clean and testable.

## View Layer (Qt6)

The view layer is implemented as a complete Qt desktop UI. It includes the main window, dashboard page, reservation management page, room management page, room status overview, and supporting dialogs such as reservation, room, invoice, confirmation, and success dialogs.

The UI layer is intentionally separated from controller logic: view classes emit user events and invoke `HotelManager` operations, while business rules and persistence remain inside the controller and model layers.

Current UI status:
- `MainWindow` provides the application shell and navigation
- `DashboardWidget` renders summary cards and charts
- `ReservationsPageWidget` handles booking filters and booking actions
- `RoomPageWidget` manages room details and room create/edit workflows
- `RoomStatusPageWidget` presents occupancy and maintenance state
- Dialog classes handle the modal create/edit flows for reservations, rooms, and invoices

The `.ui` file is compiled automatically by CMake (`AUTOUIC`). The generated `ui_MainWindow.h` is included in `MainWindow.cpp`, not edited by hand.


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

## Current Status

The current implementation provides:

✅ **Complete** — OOP design with all four pillars (encapsulation, inheritance, polymorphism, abstraction)
✅ **Complete** — Factory Pattern (RoomFactory) and Singleton Pattern (DataManager)
✅ **Complete** — Comprehensive validation and error handling in HotelManager
✅ **Complete** — Polymorphic room pricing with room metadata helpers for the UI
✅ **Complete** — Invoice generation with tax calculation, restricted to `ACTIVE`/`COMPLETED` bookings
✅ **Complete** — Booking cancellation (`cancelBooking`) with overlap-check exclusion for cancelled reservations
✅ **Complete** — Room availability (maintenance) decoupled from booking occupancy
✅ **Complete** — Dashboard/query helpers: `getRoomsByOccupancy`, `getTodayCheckIns`, `getTodayCheckOuts`, `getBookingsForCustomer`
✅ **Complete** — Integrated Qt UI with MainWindow, dashboard, reservations, room management, room status, and dialogs

**Possible future enhancements:**

- Revenue/statistics summaries over `Invoice`
- Additional invoice presentation/export options
- UX polish for the existing view layer
