# Hotel Booking Management System

**Object-Oriented Programming Project** — Group 10, Class 25C10, University of Science, Vietnam National University Ho Chi Minh City.

A C++17 / Qt6 desktop application for hotel rooms, customers, reservations, checkout, invoices, dashboard reporting, and local SQLite persistence.

## Current capabilities

- Polymorphic room portfolio: `StandardRoom`, `DeluxeRoom`, and `SuiteRoom`.
- Customer management with archive/delete workflows, country-aware ID and phone validation, and conflict highlighting in the customer list.
- Reservation creation only from Room Status, reservation editing, cancellation, checkout, and invoice generation.
- Explicit booking lifecycle: staff must record **Check-in** before a room becomes occupied, then record checkout to complete the stay.
- Reservation occupancy and room capacity validation: every booking stores adult/child counts and is rejected if it exceeds the selected room type's capacity.
- Confirmed booking rate and tax snapshots: checkout invoices are validated against the booked price, tax, and actual stay duration.
- Dated room maintenance workflows: conflict-free intervals are confirmed immediately; conflicting intervals become an internal guest-contact case until their bookings are resolved.
- Shared availability rules for Reservation and Room Status, including booking dates, active stays, archived rooms, permanent room availability, and maintenance periods.
- Room Status distinguishes a sellable `Available` room from `Awaiting check-in` (an arrival due today or overdue but not yet checked in), `Occupied`, and `Maintenance`.
- Dashboard Booking History for completed stays in the selected period, with seven entries per page.
- A4 landscape PDF export that separates current occupancy from selected-period-to-date room-night occupancy, alongside invoice-based KPIs, room inventory, actual-arrival top rooms, open bookings, cancellations/no-shows, and completed stays.
- SQLite schema migration, immutable invoice snapshots, staged loading, duplicate-customer reconciliation, and transactional persistence.
- A modal sign-in gate that starts a staff session before the operational window is opened; the current classroom bootstrap account is `admin` and should be replaced by persisted staff accounts before production use.

## Sign-in and staff context

The application loads the local database first, then requires sign-in before creating `MainWindow`. `StaffSession` keeps the authenticated username, display name, and role in memory for the running process. This prevents the old login flow from creating a second, unconfigured `MainWindow` and provides the actor context required by the forthcoming audit log.

The current project only has one classroom bootstrap account. Its password is verified as a SHA-256 digest rather than stored as plaintext, but this is not a production credential system: it has no database-backed staff records, password reset, lockout policy, or role authorization yet.

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
Upcoming ── explicit check-in ──► Active ── explicit checkout ──► Completed
    ├── cancel before planned arrival ──► Cancelled
    └── no arrival on/after planned arrival ──► No-show
```

- `cancelled`, `checkedIn`, and `checkedOut` are persisted operational facts. Planned and actual stay dates are stored separately; `createdAt` and `updatedAt` preserve booking audit timing.
- A reservation remains `Upcoming` until staff records check-in; reaching its planned arrival date alone never occupies a room.
- An overdue stay remains `Active` until checkout is explicitly recorded.
- Completed bookings leave the operational Reservations table and appear in Dashboard → Booking History when their actual checkout date is in the selected range.
- New reservations require a check-in date of today or later and `checkOutDate > checkInDate`.
- Upcoming bookings cannot be moved into the past. After check-in, the guest, room, and planned arrival cannot be changed through a normal edit.
- Checkout cannot be on/before the actual check-in date or in the future.
- Cancellation requires a reason and is allowed only before planned arrival. Staff can instead mark an unarrived, overdue reservation as a no-show; both outcomes retain their date in the booking audit record. Booking and invoice deletion are blocked to preserve history.
- A maintenance interval with no affected booking is confirmed immediately. If it overlaps an unfinished booking, the application creates an `Awaiting guest response` case and logs a `Simulated email` notice per affected reservation. The case is a soft hold: it prevents new conflicting bookings but does not present the room as under maintenance until confirmed. Staff must move, reschedule, or cancel each conflict, then explicitly confirm the case. Confirmation rechecks live booking overlap before the room is blocked. This is an internal operational log, not proof that an email was delivered; an external notification provider can replace the simulated channel later.

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
- Migration adds explicit check-in/check-out facts, actual stay dates, booked rate/tax, occupancy, and cancellation-audit columns. Legacy completed rows are backfilled only when an invoice proves checkout; a date alone never activates a stay.
- Each `Invoice` stores its own customer, room, actual-stay date, unit-price snapshot, and invoice issue date. The issue date is not a payment-status field; payment settlement requires a future folio/payment module.
- SQLite foreign keys, a 5-second busy timeout, and indexes support integrity and common booking/date queries. Customer phone numbers are unique at the database layer. `DataVersion` detects a stale full snapshot from another running instance and prevents it from overwriting newer data.
- `loadAll()` restores into a temporary `HotelManager` and replaces the live manager only after the complete snapshot is valid.
- `saveAll()` writes the current manager snapshot in one SQLite transaction. `commitChanges()` restores the last committed state after a failed save.

The current persistence strategy favors consistency and recovery over incremental writes: a successful mutation persists the complete in-memory snapshot.

## Architecture

```text
src/
├── models/        Domain entities and the room hierarchy
├── controllers/
│   ├── hotel/     HotelManager in-memory store and UI-compatible facade
│   ├── booking/   BookingManager → BookingService, InvoiceService; availability service
│   │               is created on demand by booking and view workflows
│   ├── customer/  CustomerManager → CustomerService
│   ├── room/      RoomManager → RoomService
│   └── report/    ReportService read-only reporting branch
├── database/      DataManager SQLite schema, migration, load, and save
└── views/         Qt widgets, dialogs, navigation, dashboard, and .ui files
```

`HotelManager` owns the in-memory entity collections and exposes the UI-compatible facade. Its facade methods create a `BookingManager`, `CustomerManager`, or `RoomManager` as a temporary local object for the requested workflow; it does not retain these managers as members. `ReportService` is a separately created, read-only aggregation service. See [ARCHITECTURE.md](ARCHITECTURE.md) for ownership, dependencies, and data flow.

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

Run the executable from the build directory. On a first launch, `data/hotel_data.db` is created with an empty schema; rooms, customers, bookings, and invoices are created only through the application.

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
| `Room`, `StandardRoom`, `DeluxeRoom`, `SuiteRoom` | Room hierarchy, type metadata, pricing, and maximum guest capacity |
| `RoomMaintenance` | One dated maintenance interval for a room |
| `Customer` | Guest identity, contact details, and archive state |
| `Booking` | Planned/actual stay dates, guest counts, confirmed rate/tax, and lifecycle audit facts |
| `Invoice` | Immutable billing data for one completed booking, validated against the booking snapshot |
| `HotelManager` | In-memory collections and lookup/query facade; creates workflow managers per facade call |
| `BookingManager` | Temporary booking-workflow delegate; owns booking and invoice services for its lifetime |
| `CustomerManager` | Temporary customer-workflow delegate; owns customer service for its lifetime |
| `RoomManager` | Temporary room-workflow delegate; owns room service for its lifetime |
| `ReportService` | Read-only PDF report snapshot builder |
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
