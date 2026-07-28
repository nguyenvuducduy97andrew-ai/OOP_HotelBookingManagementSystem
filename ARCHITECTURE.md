# Architecture

## Overview

Hotel Booking Management System is a C++17 / Qt6 desktop application with four cooperating layers:

```text
views  ── user interaction, rendering, signals/slots
  │
  ├── HotelManager ── in-memory collections, lookups, and UI-compatible facade
  │     ├── BookingManager
  │     │     └── BookingService, InvoiceService, RoomAvailabilityService
  │     ├── CustomerManager
  │     │     └── CustomerService
  │     └── RoomManager
  │           └── RoomService
  │
  ├── ReportService ── read-only Dashboard/PDF aggregation from HotelManager
  │
  ├── models ── Room, RoomMaintenance, Customer, Booking, Invoice
  └── DataManager ── SQLite load, migration, and atomic persistence
  │
  └── DataManager ── SQLite load, migration, and atomic persistence
```

The view layer owns presentation only. `HotelManager` owns the in-memory model collections and exposes a compatibility facade for current widgets. Focused services hold the corresponding business workflows. `DataManager` is the only component that reads or writes SQLite.

## Project layout

```text
.
├── CMakeLists.txt
├── README.md
├── ARCHITECTURE.md
├── data/
│   └── hotel_data.db             # Runtime SQLite database; not source-controlled
└── src/
    ├── main.cpp                  # Startup, database-path resolution, app shutdown save
    ├── models/                   # Domain objects and Room hierarchy
    ├── controllers/
    │   ├── hotel/                # HotelManager store/facade
    │   ├── booking/              # BookingManager
    │   │   └── services/         # Booking, Invoice, and availability workflows
    │   ├── customer/             # CustomerManager and CustomerService
    │   │   └── services/
    │   ├── room/                 # RoomManager and RoomService
    │   │   └── services/
    │   └── report/               # ReportService
    ├── database/                 # DataManager singleton
    └── views/                    # Qt widgets, dialogs, dashboard, resources, .ui files
```

`CMakeLists.txt` uses C++17, `AUTOMOC`, `AUTOUIC`, `AUTORCC`, and Qt6 Core, SQL, Widgets, and Charts.

## Startup and persistence flow

```text
main.cpp
  │
  ├── resolveDatabasePath() → data/hotel_data.db
  ├── DataManager::loadAll(hotelManager, path)
  │     ├── open SQLite and migrate schema
  │     ├── restore a temporary HotelManager snapshot
  │     └── replace live manager only after every record is valid
  ├── MainWindow(&hotelManager)
  └── QApplication event loop
          │
          ├── views call HotelManager's compatibility facade
          │     └── facade delegates booking, availability, and invoice rules to focused services
          └── views call DataManager::commitChanges() after a mutation
                   ├── saveAll() in one SQLite transaction
                   └── on failure, restoreLastSavedState()
```

`main.cpp` also calls `saveAll()` as a final shutdown safeguard. Normal UI mutations persist immediately through `commitChanges()`; users do not have to rely on application exit for data durability.

## Domain model and ownership

```text
HotelManager owns shared_ptr collections
  ├── Customer
  ├── Room
  ├── RoomMaintenance ── room number, [start date, end date), note
  ├── Booking ── weak_ptr<Customer>, weak_ptr<Room>
  └── Invoice ── weak_ptr<Booking>
```

`HotelManager` has sole ownership of model objects through `std::shared_ptr` vectors. Relationships use `std::weak_ptr`, which prevents ownership cycles and permits safe `lock()` checks when references are accessed. Supporting reservation-workflow components are friends only for controlled creation paths; they do not own the collections.

### Room hierarchy

```text
Room (abstract)
├── StandardRoom  → base rate
├── DeluxeRoom    → base rate + minibar fee
└── SuiteRoom     → base rate + premium service fee
```

`RoomFactory` creates the appropriate concrete type. Views and the controller work against the `Room` abstraction through virtual methods such as room type and pricing accessors.

### Booking and invoice relationship

```text
Customer ──┐
           ├── Booking ──► Invoice
Room ──────┘
```

- One customer can own many bookings.
- One room can occur in many bookings across different dates.
- An invoice belongs to one completed booking.
- Referential integrity is also enforced in SQLite through foreign keys.

## Booking lifecycle

```text
                  ┌──────────── cancel before check-in ────────────► Cancelled
                  │
Upcoming ──► Active ── explicit checkout + invoice ──► Completed
```

`BookingState` contains `UPCOMING`, `ACTIVE`, `COMPLETED`, and `CANCELLED`.

| State | Source of truth |
|---|---|
| `UPCOMING` | Check-in date is after today and the booking is not cancelled or checked out. |
| `ACTIVE` | Check-in date has arrived and the booking is not cancelled or checked out. A late checkout stays active. |
| `COMPLETED` | Persisted `Booking::checkedOut` is true. |
| `CANCELLED` | Persisted `Booking::cancelled` is true; this has highest priority. |

The important distinction is that planned checkout date is not completion. `BookingService::completeBooking()` records the explicit staff action, stores the actual checkout date, and sets `checkedOut = true`. Only then does the reservation leave the operational list and enter Booking History.

### Booking constraints

- New and edited reservations require `checkOutDate > checkInDate`.
- Restored legacy same-day completed stays are accepted with `checkOutDate == checkInDate`.
- Cancelled and deleted bookings do not block ordinary date-overlap checks.
- An active stay blocks a new reservation that covers today until checkout is performed.
- Cancellation is allowed only for an upcoming booking.

## Checkout and billing transaction

The Reservations page coordinates a checkout as one in-memory operation followed by one persistence operation:

```text
completeBooking()
  ├── validates active booking and actual checkout date
  ├── updates actual checkout date
  └── marks checkedOut

createInvoice()
  └── validates completed booking, tax, nights, and payment date

DataManager::commitChanges()
  ├── saveAll() inside SQLite transaction
  └── restoreLastSavedState() if save fails
```

This prevents a persisted checkout without its invoice, or an invoice attached to an uncompleted booking.

## Persistence design

### Schema

| Table | Role |
|---|---|
| `Customer` | Guest ID, name, phone number, archive state |
| `Room` | Room number, base price, type-specific fees, permanent availability/archive state |
| `RoomMaintenance` | Persisted room number, half-open maintenance interval, and optional work note |
| `Booking` | Customer/room foreign keys, dates, `cancelled`, `deleted`, `checkedOut` |
| `Invoice` | Booking foreign key, tax rate, nights, payment date |

`DataManager::initDatabase()` creates missing tables and migrates legacy columns. The `checkedOut` migration marks already historical rows (past checkout date) and bookings with an existing invoice as completed one time. Future completion always comes from the explicit checkout action.

### Load safety

`loadAll()` restores records into a temporary `HotelManager`. A bad customer, room, booking, or invoice causes load failure; it is not skipped. The live manager remains unchanged until the complete snapshot succeeds. This protects valid database records from being omitted in memory and later erased by a full save.

### Save safety

`saveAll()` serializes the complete manager snapshot inside one SQLite transaction. It validates booking and invoice object references before committing. On any statement failure, the transaction rolls back. `commitChanges()` restores the last saved state if saving fails.

## Input validation

`CountryInputRules.h` centralizes the customer ID and phone rules used by both Customer Management and Reservation dialogs.

- Supported countries: Vietnam, United States, Malaysia, United Kingdom, Japan, Singapore, South Korea, Thailand, Australia, Germany.
- Each rule has an ID regular expression, input hint, calling code, and local phone length.
- Phone normalization removes spaces, punctuation, and leading local zeroes before composing the stored international number.
- Controller validation rechecks supported ID and phone formats before persistence.

## View layer

| Component | Responsibility |
|---|---|
| `MainWindow` | Navigation shell and page composition |
| `DashboardWidget` | KPIs, charts, selected-range Booking History, and PDF export |
| `ReservationsPageWidget` | Operational reservations, checkout, cancellation, and invoice dialog |
| `CustomerPageWidget` | Customer create/edit/archive workflows |
| `RoomPageWidget` | Room create/edit/archive workflows |
| `RoomStatusPageWidget` | Occupancy and maintenance status |
| `CustomerDialog`, `ReservationDialog` | Country-aware customer/contact entry |

Completed stays are intentionally absent from `ReservationsPageWidget`; `DashboardWidget` displays them as paginated Booking History, seven records per page.

### PDF report

Dashboard PDF export uses `QPdfWriter` and `QTextDocument` in A4 landscape. It includes a selected reporting period, summary metrics, room portfolio, top rooms, open bookings, cancellations, and completed stays. The HTML uses wrapper tables and page-break classes to keep headings with their related content; Top Rooms and Completed Booking History begin on fresh pages.

`ReportService` builds the sorted reporting snapshot used by the PDF renderer. `DashboardWidget` supplies only the selected range and renders that snapshot to HTML/PDF, avoiding duplicate domain filtering inside the view.

## Layer boundaries

| Dependency | Allowed | Reason |
|---|---:|---|
| Views → `HotelManager` facade | Yes | Trigger operations without coupling current widgets to service classes. |
| Views → `DataManager` | Yes | Request persistence after a successful UI action. |
| `BookingManager` → `HotelManager` / models | Yes | Own the immediate reservation services and expose booking, checkout, and invoice operations. |
| `CustomerManager` → `HotelManager` / models | Yes | Own CustomerService and expose customer workflows. |
| `RoomManager` → `HotelManager` / models | Yes | Own RoomService and expose room workflows. |
| `ReportService` → `HotelManager` / models | Yes | Build read-only metrics and selected-period report entries. |
| `HotelManager` → models | Yes | Own and look up domain objects; retain simple read queries. |
| `DataManager` → `HotelManager` / models | Yes | Restore and serialize domain state. |
| Models → views | No | Domain entities do not depend on UI. |
| Models → `DataManager` | No | Domain entities are persistence-agnostic. |

## Design patterns and OOP principles

- **Encapsulation**: model fields are private and changed through methods.
- **Inheritance and polymorphism**: room subclasses implement room-specific pricing and metadata.
- **Abstraction**: `Room` is an abstract interface for every room type.
- **Factory**: `RoomFactory` centralizes room construction.
- **Singleton**: `DataManager` provides one shared database connection and persistence service.
- **Facade and application services**: `HotelManager` centralizes collections and backwards-compatible entry points; each direct child manager owns only its immediate service layer, while `ReportService` remains a sibling read-only reporting branch.

## Operational notes

- `Room::isAvailable` is retained for legacy permanent availability. Operational maintenance uses `RoomMaintenance` intervals (`[startDate, endDate)`) so future stays outside the closure remain valid. Both booking creation and maintenance scheduling reject overlapping intervals.
- The database is local runtime state and is ignored by Git.
- `CountryInputRules.h` is a source header and must be included when committing the project changes.
