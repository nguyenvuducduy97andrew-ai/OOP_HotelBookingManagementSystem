# Hotel Booking Management System

**Object-Oriented Programming Project** — Group 10, Class 25C10, University of Science, Vietnam National University Ho Chi Minh City.

A C++17 / Qt6 desktop application for hotel rooms, customers, reservations, checkout, invoices, dashboard reporting, and local SQLite persistence.

## Current capabilities

- Polymorphic room portfolio: `StandardRoom`, `DeluxeRoom`, and `SuiteRoom`.
- Customer management with archive/delete workflows, country-aware ID and phone validation, and conflict highlighting in the customer list.
- Reservation creation only from Room Status, reservation editing, cancellation, checkout, and invoice generation.
- Explicit booking lifecycle: a stay becomes **Completed** only after staff completes checkout.
- Dated room maintenance intervals with overlap validation against unfinished bookings.
- Shared availability rules for Reservation and Room Status, including booking dates, active stays, archived rooms, permanent room availability, and maintenance periods.
- Dashboard Booking History for completed stays in the selected period, with seven entries per page.
- A4 landscape PDF export with selected-period KPIs, room inventory, top rooms, open bookings, cancellations, and completed stays.
- SQLite schema migration, immutable invoice snapshots, staged loading, duplicate-customer reconciliation, and transactional persistence.

## Booking workflow

```text
Room Status
  └── select a room
        └── MainWindow routes the selected room to Reservation
              └── ReservationDialog opens with that room preselected
                    └── create booking and commit SQLite changes
```

The Room Status page is the only entry point for creating a reservation; the Reservations page has no separate **New Reservation** action. It displays a room's current status, then sends the selected room to the Reservations page. The dialog still validates the final customer, date range, and availability before a booking is created; the UI selection is never treated as authorization to bypass business rules.

After a successful reservation mutation, `bookingChanged` refreshes Room Status and Dashboard. A cancelled booking initiated from the Room Status flow returns the user to Room Status.

## Booking lifecycle

```text
Upcoming ── check-in date reached ──► Active ── explicit checkout ──► Completed
    │
    └── cancel before check-in ──► Cancelled
```

- `cancelled` and `checkedOut` are persisted facts.
- `Upcoming` and `Active` are derived from the check-in date and the current date.
- An overdue stay remains `Active` until checkout is explicitly recorded.
- Completed bookings leave the operational Reservations table and appear in Dashboard → Booking History when their actual checkout date is in the selected range.
- New reservations require a check-in date of today or later and `checkOutDate > checkInDate`.
- Upcoming bookings cannot be moved into the past. An active booking retains its original check-in date and cannot be backdated on checkout.
- Checkout cannot be before check-in or in the future.

Checkout and invoice creation are staged together, then persisted through one `DataManager::commitChanges()` call. If the transaction fails, the in-memory manager reloads the previous committed database snapshot.

## Availability and maintenance

`RoomAvailabilityService` is the single date-range availability source used by Reservation and Room Status.

- Cancelled, deleted, and completed bookings do not block future availability.
- An active stay blocks a requested period that covers today until the guest checks out.
- `RoomMaintenance` uses half-open intervals: `[startDate, endDate)`. A booking may begin on the maintenance end date.
- Scheduling maintenance is rejected when it overlaps an unfinished booking; creating or editing a booking is rejected when it overlaps maintenance.
- `Room::isAvailable` represents permanent availability. A dated maintenance interval is separate and does not permanently disable a room.

## Customers and country input

`src/views/CountryInputRules.h` is shared by Customer and Reservation dialogs.

- Supported countries: Vietnam, United States, Malaysia, United Kingdom, Japan, Singapore, South Korea, Thailand, Australia, and Germany.
- Each country defines an ID pattern, concise input hint, calling code, and expected local phone length.
- Phone normalization removes punctuation, spaces, and accidental local leading zeroes before storage. Stored values use international form, for example `+84912345678`.
- Customer names are not unique. Customer IDs and normalized phone numbers are protected by the customer workflow and a SQLite unique phone index. The same validation is applied when creating and editing a customer. When a conflict is detected, the conflicting customer is highlighted for the user.

### Existing database reconciliation

During database initialization, `DataManager` checks existing rows for an exact normalized **name + phone** duplicate. Before any duplicate row is removed, it creates a timestamped database copy:

```text
hotel_data.db.customer-dedup-YYYYMMDD-HHMMSSmmm.bak
```

Bookings of the removed duplicate are reassigned to the retained customer. Incomplete rows are not merged. Same-phone/different-name rows are considered ambiguous: initialization stops without deleting data, so they can be reviewed and corrected manually before the unique phone rule is enabled.

## Persistence

- Runtime database: `data/hotel_data.db`.
- Tables: `Customer`, `Room`, `RoomMaintenance`, `Booking`, and `Invoice`.
- Migration adds `Booking.checkedOut` for legacy databases. It marks a legacy booking completed only when an existing invoice proves checkout; a date alone never changes lifecycle state. Non-invoiced legacy rows remain active for staff review.
- Each `Invoice` stores its own customer, room, date, and unit-price snapshot. Future edits to a customer or room cannot change a completed invoice or completed-stay report.
- SQLite foreign keys, a 5-second busy timeout, and indexes support integrity and common booking/date queries. Customer phone numbers are unique at the database layer. `DataVersion` detects a stale full snapshot from another running instance and prevents it from overwriting newer data.
- `loadAll()` restores into a temporary `HotelManager` and replaces the live manager only after the complete snapshot is valid.
- `saveAll()` writes the current manager snapshot in one SQLite transaction. `commitChanges()` restores the last committed state after a failed save.

The current persistence strategy favors consistency and recovery over incremental writes: a successful mutation persists the complete in-memory snapshot.

## Architecture

```text
src/
├── models/        Domain entities and the room hierarchy
├── controllers/
│   ├── hotel/     HotelManager composition root and UI-compatible facade
│   ├── booking/   BookingManager → BookingService, InvoiceService, RoomAvailabilityService
│   ├── customer/  CustomerManager → CustomerService
│   ├── room/      RoomManager → RoomService
│   └── report/    ReportService read-only reporting branch
├── database/      DataManager SQLite schema, migration, load, and save
└── views/         Qt widgets, dialogs, navigation, dashboard, and .ui files
```

`HotelManager` owns the in-memory entity collections and directly owns only its three immediate managers: Booking, Customer, and Room. `ReportService` is a sibling read-only branch because reporting has its own aggregation business logic. See [ARCHITECTURE.md](ARCHITECTURE.md) for ownership, dependencies, and data flow.

## Build

### Requirements

- CMake 3.16 or newer
- Qt 6 modules: Core, Widgets, SQL, and Charts
- A C++17 compiler compatible with the selected Qt kit

### CMake

```bash
cmake -S . -B build
cmake --build build --target HotelBookingManagement
```

Run the executable from the build directory. On a first launch with an empty database, demo rooms are seeded and saved to `data/hotel_data.db`.

### Windows Qt kit note

Use one matching Qt kit end-to-end: the Qt MinGW package must be configured with its paired MinGW compiler and make program, or use a Qt MSVC package with Visual Studio. Do not mix the Qt MinGW libraries with an MSVC-generated build directory. If CMake cannot find Qt6, configure with the matching Qt prefix, for example:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64" `
  -DCMAKE_CXX_COMPILER="C:\Qt\Tools\mingw1310_64\bin\g++.exe" `
  -DCMAKE_MAKE_PROGRAM="C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe"
```

If the compiler cannot build CMake's one-file test program, repair or reinstall that MinGW kit before investigating application code.

## Main classes

| Class | Responsibility |
|---|---|
| `Room`, `StandardRoom`, `DeluxeRoom`, `SuiteRoom` | Room hierarchy, type metadata, and pricing |
| `RoomMaintenance` | One dated maintenance interval for a room |
| `Customer` | Guest identity, contact details, and archive state |
| `Booking` | Reservation dates, cancellation state, and explicit checkout state |
| `Invoice` | Billing data for one completed booking |
| `HotelManager` | In-memory collections, lookup/query facade, and manager composition root |
| `BookingManager` | Immediate owner of booking and invoice services |
| `CustomerManager` | Immediate owner of customer service |
| `RoomManager` | Immediate owner of room service |
| `ReportService` | Read-only Dashboard/PDF report snapshot builder |
| `DataManager` | SQLite schema, migrations, reconciliation, staged load, and transactional save |

## Contributors

Group 10, Class 25C10

| Student ID | Name |
|---:|---|
| 25127070 | Le Quang Khai |
| 25127190 | Nguyen Vu Duc Duy |
| 25127234 | Nguyen Thi Thao Suong |
| 25127389 | Nguyen Minh Khoi |
| 25127464 | Nguyen Kim Phuc |

## License

Educational project for OOP coursework.
