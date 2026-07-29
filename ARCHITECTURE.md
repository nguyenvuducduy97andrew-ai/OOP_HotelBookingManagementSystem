# Architecture

## Purpose

Hotel Booking Management System is a C++17 / Qt6 desktop application. Its architecture separates presentation, hotel operations, reporting, domain objects, and SQLite persistence while retaining a small compatibility facade for the existing Qt widgets.

```text
Qt views
  │ signals, dialogs, rendering
  ▼
LoginWindow ── authenticates before MainWindow is constructed
  └── StaffSession ── in-memory username, display name, and role for the active process

HotelManager ── owns in-memory model collections and exposes the UI facade
  ├── creates BookingManager temporarily for each booking facade call
  │     ├── BookingService
  │     └── InvoiceService
  ├── creates CustomerManager temporarily for each customer facade call
  │     └── CustomerService
  └── creates RoomManager temporarily for each room facade call
        └── RoomService

BookingService ── creates RoomAvailabilityService when validating a booking
ReservationDialog and RoomStatusPageWidget ── create RoomAvailabilityService
                                                  for date-range display checks

ReportService ── sibling, read-only aggregation over HotelManager
DataManager   ── SQLite schema, migration, load, reconciliation, and save
models        ── Room hierarchy, RoomMaintenance, Customer, Booking, Invoice
```

`HotelManager` does not have `BookingManager`, `CustomerManager`, or `RoomManager` member fields. Instead, its facade methods construct the relevant manager locally and delegate the request to it. Each temporary manager owns only its immediate service members during that call; `BookingManager` owns `BookingService` and `InvoiceService`, while `RoomAvailabilityService` is constructed on demand by `BookingService` and the two availability-aware views. `ReportService` is deliberately outside these mutation workflows because it has independent, read-only reporting rules.

## Project layout

```text
.
├── CMakeLists.txt
├── README.md
├── ARCHITECTURE.md
├── data/
│   └── hotel_data.db                 # Runtime SQLite database; ignored by Git
└── src/
    ├── main.cpp                      # Startup, sign-in gate, database-path resolution, shutdown safeguard
    ├── models/                       # Domain objects, StaffSession, and Room hierarchy
    ├── controllers/
    │   ├── hotel/                    # HotelManager in-memory store and facade
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
    └── views/                        # Qt widgets, LoginWindow, dialogs, .ui files, and resources
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

`RoomFactory` creates concrete room types. Most controller and view operations use the `Room` abstraction and its virtual accessors. The room-edit workflow and database restoration also use `dynamic_pointer_cast` when they must write subtype-specific fees (`DeluxeRoom::miniBarFee` or `SuiteRoom::premiumServiceFee`).

## Business boundaries

| Component | Direct responsibility | Does not own |
|---|---|---|
| `HotelManager` | Collections, lookups, UI-facing facade, and temporary manager creation | Retained manager or service objects |
| `BookingManager` | Temporary delegation of reservation, checkout, and invoice use cases | `RoomAvailabilityService`, customer, or room service layers |
| `BookingService` | Booking date, capacity, explicit check-in/out, cancellation-audit validation; on-demand availability checks | Persistence or report rendering |
| `InvoiceService` | Invoice validation and creation after checkout against locked booking tax/rate and actual duration | SQLite operations |
| `RoomAvailabilityService` | Shared booking/maintenance availability calculation | UI navigation or persistence |
| `CustomerManager` / `CustomerService` | Customer validation, conflict detection, archive/delete workflows | Booking, room, or report workflows |
| `RoomManager` / `RoomService` | Room registration and dated maintenance workflows | Booking, customer, or report workflows |
| `ReportService` | Read-only selected-period Dashboard/PDF data snapshot | Mutation and persistence |
| `DataManager` | SQLite migration, reconciliation, staged load, and atomic snapshot save | UI decisions and booking rules |

This structure keeps `HotelManager` as an in-memory facade rather than a persistent service container. The customer and room branches can still grow into focused modules without adding lower-level service fields to `HotelManager`.

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
  └── calls HotelManager → temporary BookingManager → BookingService
  ▼
DataManager::commitChanges()
  ├── save complete snapshot in one SQLite transaction
  └── restore previous snapshot if the save fails
```

`bookingChanged` refreshes Dashboard and Room Status after a successful reservation mutation. The selected room is only a UI convenience; final availability is always recalculated by the booking service.

### Checkout and invoice

```text
ReservationsPageWidget
  ├── HotelManager::checkInBooking() records today's arrival before occupancy can become active
  ├── HotelManager::completeBooking()
  │     └── temporary BookingManager → BookingService validates active state and actual checkout date
  ├── HotelManager::createInvoice()
  │     └── temporary BookingManager → InvoiceService validates completed booking, invoice issue date, one invoice per booking,
  │         and captures immutable guest/room/date/price data
  └── DataManager::commitChanges()
```

Check-in sets `Booking::checkedIn` and an actual arrival date; only then does the booking become `Active`. Checkout sets `Booking::checkedOut` and an actual departure date while preserving planned dates. The booking then moves out of operational reservations and into Dashboard Booking History. Invoice totals are derived from the locked booking rate/tax and actual duration; completed-stay report identity fields use the immutable invoice snapshot. The UI performs one persistence commit for the checkout-plus-invoice pair.

### Customer creation and conflict handling

```text
CustomerDialog → CustomerPageWidget → HotelManager
  → temporary CustomerManager → CustomerService
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
| `UPCOMING` | Not cancelled, not checked out, and no persisted check-in event | May be edited or cancelled |
| `ACTIVE` | Persisted `checkedIn == true`; not cancelled or checked out | Blocks present-day occupancy until checkout |
| `COMPLETED` | Persisted `checkedOut == true` | Listed in history; not editable or cancellable |
| `CANCELLED` | Persisted `cancelled == true` | Does not block availability |

An overdue departure remains `ACTIVE` until staff checks it out. Planned dates are never used to infer check-in or completion during normal operation.

## Persistence design

### Schema and indexes

| Table | Purpose |
|---|---|
| `Customer` | Customer ID, name, phone number, archive state |
| `Room` | Room number, price, room type, fees, permanent availability/archive state |
| `RoomMaintenance` | Room number, maintenance ID, interval, optional note, case status, and creation timestamp |
| `MaintenanceGuestNotice` | Internal simulated guest-contact log for a maintenance case and affected booking |
| `Booking` | Customer/room keys, planned/actual dates, adult/child counts, booked rate/tax, check-in/out/cancellation facts, and creation/update audit timestamps |
| `Invoice` | Booking key, tax, nights, invoice issue date, and immutable customer/room/actual-date/price snapshot |

SQLite foreign keys preserve customer/room/booking relationships. A connection-level 5-second busy timeout reduces transient lock failures. Query indexes cover booking room/date lookup, booking customer lookup, and maintenance room/date lookup; a unique partial index enforces non-empty customer phone numbers. A maintenance conflict produces a persisted `Awaiting guest response` case plus simulated internal notices. That case is a soft hold for new reservations but does not change the live room status until confirmed; a separate confirmation operation rechecks live overlap before it becomes a room-blocking maintenance interval. `DataVersion` carries a monotonically increasing revision, so a stale full snapshot from another application instance is rejected rather than overwriting newer data.

### Migration and duplicate reconciliation

`DataManager::initDatabase()` migrates explicit check-in/out, actual-date, confirmed pricing/tax, capacity, and cancellation-audit fields. The one-time migration marks a row completed only when an existing invoice proves checkout; a planned date is never treated as proof of occupancy. Legacy invoices receive a one-time snapshot from linked customer, room, and booking records before immutable invoice loading is enforced.

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

`ReportService` builds a `DashboardReportData` value from `HotelManager` without mutating it. The PDF-export workflow separates the real-time occupancy snapshot from selected-period-to-date occupancy, calculated as actual occupied room-nights divided by available room-nights. It also includes room counts, actual-arrival top rooms, open bookings, cancellations/no-shows, completed stays, and invoice-based revenue KPIs (invoiced revenue, ADR, RevPAR). It does not interpret an invoice issue date as proof of payment. Completed stays prefer immutable invoice snapshots; other statuses use the current linked model. It sorts data before `DashboardWidget` renders the PDF.

`DashboardWidget` owns interactive dashboard presentation, including its own live card, chart, popular-room, and booking-history aggregation over `HotelManager`. It also renders the PDF HTML, but delegates PDF report aggregation to `ReportService`. PDF export uses A4 landscape pages and print-safe wrapper blocks so each heading stays with its related table; Top Rooms and Completed Booking History begin on new pages.

## Dependency rules

| Dependency | Allowed | Reason |
|---|---:|---|
| Views → `HotelManager` facade | Yes | Existing widgets can call stable UI-facing operations. |
| Views → `DataManager` | Yes | Views request persistence only after a successful business mutation. |
| `HotelManager` → workflow managers | Yes | Facade methods create a local manager and delegate one workflow; no manager is retained as a member. |
| Manager → direct services | Yes | A temporary manager owns its immediate service members. `BookingManager` owns only booking and invoice services. |
| `BookingService` / availability-aware views → `RoomAvailabilityService` | Yes | The service is constructed on demand for booking validation and date-range display checks. |
| `ReportService` → `HotelManager` / models | Yes | Read-only aggregation. |
| `DataManager` → `HotelManager` / models | Yes | Restore and serialize domain state. |
| Models → views or `DataManager` | No | Domain objects remain independent of UI and persistence. |
| Services → views | No | Business rules do not depend on dialogs or widgets. |

## OOP and design patterns

- **Encapsulation**: entities expose controlled methods rather than public mutable fields.
- **Inheritance and polymorphism**: concrete room types implement type-specific behavior through `Room`.
- **Factory**: `RoomFactory` centralizes concrete room construction.
- **Facade**: `HotelManager` preserves a stable boundary for current UI code while creating temporary workflow managers to delegate operations.
- **Application services**: focused services implement booking, invoice, customer, room, maintenance, and availability workflows.
- **Singleton**: `DataManager` provides one shared SQLite persistence component.
- **DTO/report snapshot**: `DashboardReportData` separates PDF-report aggregation from PDF rendering.
