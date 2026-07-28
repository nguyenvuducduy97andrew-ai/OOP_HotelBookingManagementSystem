# Architecture

## Purpose

Hotel Booking Management System is a C++17 / Qt6 desktop application. Its architecture separates presentation, hotel operations, reporting, domain objects, and SQLite persistence while retaining a small compatibility facade for the existing Qt widgets.

```text
Qt views
  │ signals, dialogs, rendering
  ▼
HotelManager ── owns in-memory model collections and three direct managers
  ├── BookingManager
  │     ├── BookingService
  │     ├── InvoiceService
  │     └── RoomAvailabilityService
  ├── CustomerManager
  │     └── CustomerService
  └── RoomManager
        └── RoomService

ReportService ── sibling, read-only aggregation over HotelManager
DataManager   ── SQLite schema, migration, load, reconciliation, and save
models        ── Room hierarchy, RoomMaintenance, Customer, Booking, Invoice
```

`HotelManager` does not directly own services two levels below it. It directly owns only `BookingManager`, `CustomerManager`, and `RoomManager`; each manager owns only its immediate service layer. `ReportService` is deliberately outside that hierarchy because it has independent, read-only reporting rules rather than hotel mutation workflows.

## Project layout

```text
.
├── CMakeLists.txt
├── README.md
├── ARCHITECTURE.md
├── data/
│   └── hotel_data.db                 # Runtime SQLite database; ignored by Git
└── src/
    ├── main.cpp                      # Startup, database-path resolution, shutdown safeguard
    ├── models/                       # Domain objects and Room hierarchy
    ├── controllers/
    │   ├── hotel/                    # HotelManager composition root and facade
    │   ├── booking/
    │   │   ├── BookingManager.*
    │   │   └── services/             # Booking, invoice, and availability workflows
    │   ├── customer/
    │   │   ├── CustomerManager.*
    │   │   └── services/             # Customer workflow
    │   ├── room/
    │   │   ├── RoomManager.*
    │   │   └── services/             # Room and maintenance workflow
    │   └── report/                   # ReportService and report DTOs
    ├── database/                     # DataManager singleton
    └── views/                        # Qt widgets, dialogs, .ui files, and resources
```

`CMakeLists.txt` enables C++17, `AUTOMOC`, `AUTOUIC`, `AUTORCC`, and links Qt6 Core, SQL, Widgets, and Charts.

## Ownership and relationships

```text
HotelManager owns vectors of shared_ptr
  ├── Customer
  ├── Room
  ├── Booking ── weak_ptr<Customer>, weak_ptr<Room>
  └── Invoice ── weak_ptr<Booking> plus immutable customer/room/date/price snapshot

HotelManager owns vector values
  └── RoomMaintenance ── room number, [startDate, endDate), note
```

The manager is the sole owner of domain entities. Cross-entity links use `std::weak_ptr` so bookings and invoices do not create ownership cycles. Services are friends only where controlled construction or mutation needs access to the manager-owned collections; services do not own collections.

### Room hierarchy

```text
Room (abstract)
├── StandardRoom  → base rate
├── DeluxeRoom    → base rate + minibar fee
└── SuiteRoom     → base rate + premium service fee
```

`RoomFactory` creates concrete room types. Controllers and views use the `Room` abstraction and virtual accessors instead of coupling normal workflows to room subclasses.

## Business boundaries

| Component | Direct responsibility | Does not own |
|---|---|---|
| `HotelManager` | Collections, lookups, query facade, manager composition | Booking/customer/room service objects directly |
| `BookingManager` | Reservation, checkout, and invoice use cases | Customer or room service layers |
| `BookingService` | Booking date/state validation and mutations | Persistence or report rendering |
| `InvoiceService` | Invoice validation and creation after checkout | SQLite operations |
| `RoomAvailabilityService` | Shared booking/maintenance availability calculation | UI navigation or persistence |
| `CustomerManager` / `CustomerService` | Customer validation, conflict detection, archive/delete workflows | Booking, room, or report workflows |
| `RoomManager` / `RoomService` | Room registration and dated maintenance workflows | Booking, customer, or report workflows |
| `ReportService` | Read-only selected-period Dashboard/PDF data snapshot | Mutation and persistence |
| `DataManager` | SQLite migration, reconciliation, staged load, and atomic snapshot save | UI decisions and booking rules |

This structure allows the customer branch to grow into a customer-care module and the room branch to grow into a room/maintenance module without expanding `HotelManager` into a service container for all lower-level behavior.

## Main data flows

### Application startup

```text
main.cpp
  │
  ├── resolve database path → data/hotel_data.db
  ├── DataManager::loadAll(hotelManager, path)
  │     ├── open SQLite and create/migrate tables
  │     ├── create query-supporting indexes
  │     ├── reconcile exact legacy customer duplicates safely
  │     ├── restore Customer, Room, Booking, RoomMaintenance, and Invoice
  │     │   into a temporary HotelManager
  │     └── replace the live HotelManager only after all rows are valid
  ├── MainWindow(&hotelManager)
  └── Qt event loop
```

`loadAll()` never skips an invalid row. A failed load leaves the active in-memory manager unchanged, preventing a later full save from silently deleting data that was not loaded.

### Create a reservation from Room Status

```text
RoomStatusPageWidget
  ├── user selects a room
  ├── emits bookingRequested(roomNumber)
  ▼
MainWindow
  ├── opens Reservations page (which has no standalone creation action)
  └── calls startNewReservationForRoom(roomNumber)
  ▼
ReservationDialog
  ├── preselects the room
  ├── uses RoomAvailabilityService for final date-range validation
  └── calls HotelManager → BookingManager → BookingService
  ▼
DataManager::commitChanges()
  ├── save complete snapshot in one SQLite transaction
  └── restore previous snapshot if the save fails
```

`bookingChanged` refreshes Dashboard and Room Status after a successful reservation mutation. The selected room is only a UI convenience; final availability is always recalculated by the booking service.

### Checkout and invoice

```text
ReservationsPageWidget
  ├── BookingManager::completeBooking()
  │     └── BookingService validates active state and actual checkout date
  ├── BookingManager::createInvoice()
  │     └── InvoiceService validates completed booking, payment date, one invoice per booking,
  │         and captures immutable guest/room/date/price data
  └── DataManager::commitChanges()
```

Checkout sets `Booking::checkedOut`. The booking becomes `Completed` only from that persisted fact, then moves out of operational reservations and into Dashboard Booking History. Invoice totals and completed-stay report identity fields use the persisted invoice snapshot, not mutable Customer or Room data. The UI performs one persistence commit for the checkout-plus-invoice pair.

### Customer creation and conflict handling

```text
CustomerDialog → CustomerPageWidget → HotelManager
  → CustomerManager → CustomerService
      ├── validate ID and normalized international phone
      ├── reject a duplicate phone number during both creation and editing
      ├── reject a duplicate customer ID
      ├── return the conflicting customer ID for UI highlighting
      └── commit SQLite changes after successful mutation
```

Names are intentionally not unique. The system treats duplicate phone and customer ID values as account conflicts, while allowing different customers to share a name.

### Availability and maintenance

`RoomAvailabilityService` performs one scan over maintenance intervals and bookings, builds an unavailable-room set, and returns rooms that are usable for the requested date range. Reservation and Room Status use this same result, preventing the two pages from applying different availability rules.

`RoomMaintenance` is a half-open interval `[startDate, endDate)`. Both booking creation/update and maintenance scheduling reject overlaps with unfinished work or stays. Completed, cancelled, and deleted bookings do not reserve future availability.

## Booking state model

| State | Source of truth | Operational behavior |
|---|---|---|
| `UPCOMING` | Check-in is after today; not cancelled or checked out | May be edited or cancelled |
| `ACTIVE` | Check-in has arrived; not cancelled or checked out | Blocks present-day occupancy until checkout |
| `COMPLETED` | Persisted `checkedOut == true` | Listed in history; not editable or cancellable |
| `CANCELLED` | Persisted `cancelled == true` | Does not block availability |

An overdue departure remains `ACTIVE` until staff checks it out. Planned checkout is not used to infer completion during normal operation.

## Persistence design

### Schema and indexes

| Table | Purpose |
|---|---|
| `Customer` | Customer ID, name, phone number, archive state |
| `Room` | Room number, price, room type, fees, permanent availability/archive state |
| `RoomMaintenance` | Room number, maintenance ID, interval, optional note |
| `Booking` | Customer/room keys, dates, cancelled/deleted/checked-out flags |
| `Invoice` | Booking key, tax, nights, payment date, and immutable customer/room/date/price snapshot |

SQLite foreign keys preserve customer/room/booking relationships. A connection-level 5-second busy timeout reduces transient lock failures. Query indexes cover booking room/date lookup, booking customer lookup, and maintenance room/date lookup; a unique partial index enforces non-empty customer phone numbers. `DataVersion` carries a monotonically increasing revision, so a stale full snapshot from another application instance is rejected rather than overwriting newer data.

### Migration and duplicate reconciliation

`DataManager::initDatabase()` adds the explicit `Booking.checkedOut` column when needed. The one-time migration marks a row completed only when an existing invoice proves checkout; a passed planned checkout date is never treated as proof. Legacy invoices receive a one-time snapshot from their linked customer, room, and booking records before immutable invoice loading is enforced.

On initialization, the data manager also scans for exact normalized **customer name + phone** duplicates. If duplicates exist, it:

1. checkpoints SQLite and copies the database to a timestamped `.customer-dedup-*.bak` file;
2. keeps the active, earliest row as canonical;
3. repoints its historical bookings; and
4. removes only the unambiguous duplicate row inside a transaction.

Incomplete rows are intentionally left unchanged. Same-phone/different-name rows stop initialization without a destructive write, because they are ambiguous and must be reviewed manually. Only after that check does SQLite enable the unique customer-phone index.

### Save and recovery

```text
DataManager::commitChanges(manager)
  ├── saveAll(manager)
  │     ├── compare DataVersion against the revision loaded into memory
  │     ├── begin transaction
  │     ├── replace the persisted snapshot
  │     ├── advance DataVersion
  │     └── commit or rollback
  └── on failure: loadAll(manager, last saved path)
```

The application currently saves the complete manager snapshot rather than individual row deltas. This keeps the current model/database synchronization and rollback behavior simple and atomic; a future high-scale version can replace this with repository-level incremental writes without changing view contracts.

## Reporting and PDF export

`ReportService` builds a `DashboardReportData` value from `HotelManager` without mutating it. The report contains selected-period labels, KPIs, room counts, occupancy, top rooms, open bookings, cancellations, and completed stays. Completed stays prefer immutable invoice snapshots; other statuses use the current linked model. It sorts data before `DashboardWidget` renders it.

`DashboardWidget` owns presentation and PDF HTML generation only. PDF export uses A4 landscape pages and print-safe wrapper blocks so each heading stays with its related table; Top Rooms and Completed Booking History begin on new pages.

## Dependency rules

| Dependency | Allowed | Reason |
|---|---:|---|
| Views → `HotelManager` facade | Yes | Existing widgets can call stable UI-facing operations. |
| Views → `DataManager` | Yes | Views request persistence only after a successful business mutation. |
| `HotelManager` → direct managers | Yes | Composition of immediate hotel operation branches. |
| Manager → direct services | Yes | One-level ownership boundary. |
| `ReportService` → `HotelManager` / models | Yes | Read-only aggregation. |
| `DataManager` → `HotelManager` / models | Yes | Restore and serialize domain state. |
| Models → views or `DataManager` | No | Domain objects remain independent of UI and persistence. |
| Services → views | No | Business rules do not depend on dialogs or widgets. |

## OOP and design patterns

- **Encapsulation**: entities expose controlled methods rather than public mutable fields.
- **Inheritance and polymorphism**: concrete room types implement type-specific behavior through `Room`.
- **Factory**: `RoomFactory` centralizes concrete room construction.
- **Facade**: `HotelManager` preserves a stable boundary for current UI code while delegating workflows.
- **Application services**: focused services implement booking, invoice, customer, room, maintenance, and availability workflows.
- **Singleton**: `DataManager` provides one shared SQLite persistence component.
- **DTO/report snapshot**: `DashboardReportData` separates report aggregation from rendering.
