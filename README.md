# Hotel Booking Management System

**Object-Oriented Programming Project** — Group 10, Class 25C10, University of Science, Vietnam National University Ho Chi Minh City.

A C++17 / Qt6 desktop application for hotel rooms, customers, reservations, checkout, invoices, dashboard reporting, and local SQLite persistence.

> [TIME_BASED_BOOKING_SPEC.md](TIME_BASED_BOOKING_SPEC.md) is the authoritative rule set for the planned/actual time model implemented by this project.

## Current capabilities

- Polymorphic room portfolio: `StandardRoom`, `DeluxeRoom`, and `SuiteRoom`.
- Customer management with archive/delete workflows, searchable/filterable/sortable lists, country-aware ID and phone validation, conflict highlighting, and a selected-customer booking-history panel.
- Reservation customer picker searches active guests by document number, name, or phone, then reuses their stored identity key without re-registering them.
- Reservation creation only from Room Status, upcoming-reservation editing, active-stay extension, cancellation, checkout, and invoice generation.
- Room selection uses a review-first flow: a Room Status card opens `RoomInfoDialog`, and only its explicit **Booking** action opens `ReservationDialog` with the room and selected schedule prefilled.
- Standard, Deluxe, and Suite room reviews use a shared image carousel: one centered photo, softly faded adjacent previews, previous/next controls, and an image-position label. The same gallery is used by Room Management and the booking review step.
- One shared compact schedule picker presents all seven weekdays and 24 whole-hour slots without horizontal scrolling. Check-in remains the editable endpoint until staff switches to Check-out.
- Explicit booking lifecycle: staff must record **Check-in** before a room becomes occupied, then record checkout to complete the stay.
- Reservation occupancy and room capacity validation: every booking stores adult/child counts and is rejected if it exceeds the selected room type's capacity.
- Confirmed booking rate and tax snapshots: checkout invoices are validated against the booked price, tax, and actual stay duration.
- Dated room maintenance workflows: conflict-free intervals are confirmed immediately; conflicting intervals become an internal guest-contact case until their bookings are resolved.
- Shared timestamp availability rules for Reservation and Room Status, including booking intervals, the two-hour turnover buffer, Cleaning, soft holds, archived rooms, permanent room availability, and Maintenance periods.
- Room Status distinguishes a sellable `Available` room from `Awaiting check-in` (an arrival due today or overdue but not yet checked in), `Occupied`, and `Maintenance`.
- Dashboard Booking History for completed stays in the selected period, with seven entries per page.
- A dedicated Statistics page for visualizing monthly invoice revenue, today's revenue composition, booking distribution by weekday, and booked room types, with a searchable and filterable invoice list and invoice-detail action.
- A4 landscape PDF export with measured, whole-section pagination, planned/actual timestamps, invoice-based hourly KPIs, saleable room-hour occupancy, room inventory, actual-arrival top rooms, completed stays, and a dedicated reason column for cancellations.
- SQLite schema migration, immutable invoice snapshots, staged loading, explicit administrator-only duplicate reconciliation, and transactional upsert persistence.
- A modal sign-in gate that starts a staff session before the operational window is opened, including a password-visibility control and a single validation message per failed sign-in; the current classroom bootstrap account is `admin` and should be replaced by persisted staff accounts before production use.

## Sign-in and staff context

The application loads the local database first, then requires sign-in before creating `MainWindow`. `StaffSession` keeps the authenticated username, display name, and role in memory for the running process. This prevents the old login flow from creating a second, unconfigured `MainWindow` and provides the actor context required by the forthcoming audit log.

The current project only has one classroom bootstrap account. Its password is verified as a SHA-256 digest rather than stored as plaintext, but this is not a production credential system: it has no database-backed staff records, password reset, lockout policy, or role authorization yet.

## Booking workflow

```text
Room Status
  └── choose schedule and occupancy
        └── select an available room card
              └── RoomInfoDialog shows room facts and amenities
                    └── Booking
                          └── MainWindow opens ReservationDialog with room/schedule preselected
                                └── create booking and commit SQLite changes
```

The Room Status page is the only entry point for creating a reservation; the Reservations page has no separate **New Reservation** action. Clicking a candidate room first opens a read-only room review. Closing that dialog has no booking effect; only **Booking** advances to Reservation. The reservation dialog still validates the final customer, timestamp range, capacity, and availability before creation, so neither the card selection nor room review bypasses business rules.

After a successful reservation mutation, `bookingChanged` refreshes Room Status and Dashboard. A cancelled booking initiated from the Room Status flow returns the user to Room Status.

### Schedule and dialog interaction

- The schedule picker has explicit `Check-in` and `Check-out` modes. While `Check-in` is active, every date/hour click replaces the previous arrival and resets the provisional checkout to the one-hour minimum. A longer interval is created only after switching to `Check-out`.
- Week numbers are not shown. The calendar and the 6-by-4 hour grid are centered compact surfaces; unavailable dates and hours remain disabled and grey.
- Functional dialogs use an opaque white surface, a light-blue outer shell around the complete custom title bar, rounded top corners, fixed bounded sizes, and draggable headers. They cannot be freely resized or exceed their owner window.
- Mouse-wheel gestures do not change non-monetary combo boxes, dates, times, counts, or measurements inside data-entry dialogs. This keeps scrolling deterministic. Monetary controls explicitly marked as prices or fees retain wheel adjustment.

## Booking lifecycle

```text
Upcoming ── explicit check-in ──► Active ── explicit checkout ──► Completed
    └── cancel before actual check-in ──► Cancelled
```

- `cancelled`, `checkedIn`, and `checkedOut` are persisted operational facts. Planned and actual stay timestamps are stored separately; `createdAt` and `updatedAt` preserve booking audit timing.
- A reservation remains `Upcoming` until staff records check-in; reaching its planned arrival date alone never occupies a room.
- An overdue stay remains `Active` until checkout is explicitly recorded.
- Completed bookings leave the operational Reservations table and appear in Dashboard → Booking History when their actual checkout date is in the selected range.
- New reservations use planned whole-hour timestamps, must be at least one hour long, and require a two-hour turnover buffer after the preceding stay.
- Upcoming bookings cannot be moved into the past. After check-in, the guest and room are immutable; staff may use the dedicated **Extend stay** action to move only planned checkout later when the interval and its Cleaning buffer remain free.
- Checkout records an actual timestamp. Billing is based on actual elapsed seconds, rounded half-up to billable hours with a one-hour minimum; same-day checkout is therefore supported normally.
- Cancellation requires a reason and is permitted only before actual check-in. A guest who did not arrive is represented by a cancellation reason, not a second lifecycle state. Booking and invoice deletion are blocked to preserve history.
- A maintenance interval with no affected booking is confirmed immediately. If it overlaps an unfinished booking, the application creates an `Awaiting guest response` case and logs a `Simulated email` notice per affected reservation. The case is a soft hold: it prevents new conflicting bookings but does not present the room as under maintenance until confirmed. Staff must move, reschedule, or cancel each conflict, then explicitly confirm the case. Confirmation rechecks live booking overlap before the room is blocked. This is an internal operational log, not proof that an email was delivered; an external notification provider can replace the simulated channel later.

Checkout and invoice creation are staged together, then persisted through one `DataManager::commitChanges()` call. If the transaction fails, the in-memory manager reloads the previous committed database snapshot.

## Availability and maintenance

`BookingManager` is the single date-range availability authority. Reservation and Room Status request availability through `HotelManager`, which supplies canonical room and maintenance data without exposing domain managers to views.

- Cancelled, deleted, and completed bookings do not block future availability.
- Availability uses half-open timestamp intervals `[start, end)`. Active stays block a room until actual checkout; Cleaning starts then and blocks it for two hours unless staff marks it ready earlier.
- Planned consecutive reservations require a two-hour turnover buffer. Confirmed Maintenance and an `Awaiting guest response` Maintenance soft hold also reject a conflicting new reservation.
- `RoomMaintenance` uses half-open intervals. User-selected Maintenance dates are full-day inclusive and are persisted with the following midnight as the exclusive end.
- Before checkout, reception sees a late-checkout warning after the 30-minute grace point and an **Arrival at risk** warning naming any next booking that Cleaning may delay. Before a Maintenance hold is created, affected upcoming/active reservations are listed for confirmation.
- A conflict-free maintenance interval is confirmed immediately. An interval that overlaps an unfinished booking becomes an `Awaiting guest response` soft-hold case with simulated internal notices; it still blocks new overlapping reservations but becomes visible as live maintenance only after staff resolve conflicts and confirm it.
- `Room::isAvailable` represents permanent availability. A dated maintenance interval is separate and does not permanently disable a room.

## Customers and country input

`src/models/customer/CountryInputRules.h` is shared by Customer and Reservation dialogs.

- Supported countries: Vietnam, United States, Malaysia, United Kingdom, Japan, Singapore, South Korea, Thailand, Australia, and Germany.
- Each identity document stores a type (`National ID`, `Passport`, or `Other`), issuing country, and document number. Customer identity is keyed by that combination, so the same number from different issuing countries does not collide.
- Phone checks implement documented **supported formats** for these ten profiles, including variable-length ranges where applicable. They are intentionally not a complete numbering-plan implementation for every carrier or country.
- Phone normalization removes punctuation, spaces, and accidental local leading zeroes before storage. Stored values use international form, for example `+84912345678`.
- Customer name is captured as one required **full legal name**. The validation accepts a one-part legal name, all-uppercase document names, and scripts without uppercase/lowercase; it does not force a Western surname/given-name structure.
- Customer names are not unique. Customer IDs and normalized phone numbers are protected by the customer workflow and a SQLite phone lookup index. The same validation is applied when creating and editing a customer. When a conflict is detected, the conflicting customer is highlighted for the user.
- Reservation provides an existing-customer picker. Selecting a guest reuses the stored internal customer key and locks the copied identity/contact fields, including legacy records that cannot be reconstructed from the supported country selectors. Staff select `New customer — enter details manually` only when creating a genuinely new customer.
- Selecting a customer row opens `Customer reservations` directly below the list. This read-only all-status customer-profile view is distinct from Dashboard Booking History, which contains completed stays in the selected reporting period. It contains planned/actual departure facts, room, lifecycle status, and cancellation reason; booking mutations remain in Reservation Management.

### Existing database reconciliation

Normal startup never merges, archives, deletes, or copies customer records. New and edited customers are validated by the application rules, while the database keeps a non-unique phone lookup index so historical records can load without destructive "repair". `DataManager::reconcileCustomerDuplicates()` remains an explicit maintenance-only operation for an administrator who has reviewed a legacy database; it is not part of the operational startup path.

## Persistence

- Runtime database: `data/hotel_data.db` in the project root. The application creates the `data` folder and an empty SQLite database there when absent.
- The application never copies a database to or from the Windows application-data folder; the project file is the single runtime data source.
- Tables: `Customer`, `Room`, `RoomMaintenance`, `MaintenanceGuestNotice`, `Booking`, and `Invoice`.
- Migration adds explicit check-in/check-out facts, actual stay dates, booked rate/tax, occupancy, and cancellation-audit columns. Legacy completed rows are backfilled only when an invoice proves checkout; a date alone never activates a stay.
- Checkout requires one recorded payment with a method, amount, and received date. The invoice stores this payment fact separately from its issue date; a remaining balance is shown when the received amount is partial.
- SQLite foreign keys, a 5-second busy timeout, and indexes support integrity and common booking/date queries. Customer phone validation is enforced by the domain rules; the phone index supports lookup without silently rewriting historical records. `DataVersion` detects a stale write from another running instance and prevents it from overwriting newer data.
- `loadAll()` restores into a temporary `HotelManager` and replaces the live manager only after the complete snapshot is valid.
- `saveAll()` upserts the current in-memory records and deletes only records explicitly removed from the canonical collections, all in one SQLite transaction. `commitChanges()` restores the last committed state after a failed save.

The current persistence strategy favors consistency and recovery: it still validates a complete canonical in-memory graph, but avoids deleting and rebuilding every table for an ordinary mutation.

## Architecture

```text
src/
├── models/        Domain entities and the room hierarchy
├── controllers/
│   ├── hotel/     HotelManager facade and cross-domain coordinator
│   ├── booking/   BookingManager: bookings, invoices, lifecycle, availability
│   ├── customer/  CustomerManager: customers and identity/collision rules
│   ├── room/      RoomManager: rooms, maintenance, and notices
│   └── report/    ReportService read-only reporting branch
├── database/      DataManager SQLite schema, migration, load, and save
├── resources/     Small embedded icons and room-photo galleries
└── views/         Qt widgets, dialogs, navigation, dashboard, and .ui files
```

`HotelManager` owns one persistent `RoomManager`, `CustomerManager`, and `BookingManager`. Each manager is the sole owner of its domain collections; `HotelManager` keeps no duplicate collections. Single-domain operations are forwarded to the relevant manager, while workflows involving multiple domains are coordinated by the facade. Views and `ReportService` never access managers directly. See [ARCHITECTURE.md](ARCHITECTURE.md) for ownership, dependencies, and data flow.

## Build

### Requirements

- CMake 3.17 or newer
- Qt 6 modules: Core, Widgets, SQL, Charts, and SVG
- A C++17 compiler compatible with the selected Qt kit

### CMake

```bash
cmake -S . -B build
cmake --build build --target HotelBookingManagement
```

Room photos are discovered from `src/resources/room_images` and embedded with Qt `BIG_RESOURCES`. This generates direct object data rather than compiling a multi-megabyte C++ byte array. JPEG compression is not repeated during resource generation. The optimization is part of `CMakeLists.txt`, so a normal CMake build receives it on every supported machine without a personal path, plugin, or environment setting. `resources.qrc` remains intentionally small and contains only application icons.

At runtime, `RoomImageCarousel` loads only the selected room type, decodes each JPEG close to its display size, caches display-ready pixmaps, debounces resize-driven refreshes, and skips an unchanged render signature. The original photo dimensions therefore do not need to be expanded into memory on every room selection.

### Business tests

The repository includes a headless CTest executable (`HotelBookingManagementBusinessTests`) for rules that must not depend on widget interaction. Enable and run it with:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target HotelBookingManagementBusinessTests
ctest --test-dir build --output-on-failure
```

It verifies same-day and overnight booking, the two-hour adjacent-slot buffer, Cleaning and early room-ready behavior, full-day Maintenance, early check-in/early checkout, hourly rounding at 29/30/31-minute boundaries, room moves, self-excluding edits, cancellation, existing/new-customer safeguards, locked base-rate quotes after later room/optional-fee changes, and legacy database migration. The test database is temporary and never uses `data/hotel_data.db`. Optional service line items are a future feature, so no service ledger exists to test yet.

Run the executable with its working directory inside the project tree (the root or a build subfolder). The application resolves the project root, then uses only `data/hotel_data.db`. If that file does not exist, SQLite creates an empty schema there; rooms, customers, bookings, and invoices are created only through the application.

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
| `MaintenanceGuestNotice` | Internal simulated guest-contact log for an affected maintenance booking; it is not delivery evidence |
| `StaffSession` | In-memory authenticated username, display name, and role for the current desktop process |
| `HotelManager` | Stable UI/persistence facade; owns managers and coordinates cross-domain workflows |
| `BookingManager` | Owns bookings and invoices; handles lifecycle, availability, billing, and restoration |
| `CustomerManager` | Owns customers; handles identity validation, collisions, archive, restore, and lookup |
| `RoomManager` | Owns rooms, maintenance, and notices; handles room-local and maintenance-local rules |
| `ReportService` | Read-only PDF report snapshot builder |
| `DataManager` | SQLite schema, migrations, explicit reconciliation tool, staged load, and transactional upsert save |
| `StatisticsPageWidget` | Invoice and booking data visualization, invoice filtering, and invoice-detail access |
| `RoomImageCarousel` | Shared Standard/Deluxe/Suite gallery with lazy type selection, scaled decoding, cached crops, and resize debouncing |

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
