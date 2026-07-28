# Hotel Booking Management System

**Object-Oriented Programming Project** — Group 10, Class 25C10, University of Science, Vietnam National University Ho Chi Minh City.

A C++17 / Qt6 desktop application for managing rooms, guests, reservations, checkout, invoices, dashboard reporting, and SQLite-backed persistence.

## Highlights

- Three polymorphic room types: Standard, Deluxe, and Suite.
- Customer and reservation management with validation, archival, filtering, and duplicate prevention.
- Country-aware customer ID and phone input for 10 supported countries. Phone input removes formatting and accidental leading zeroes before storage.
- Explicit checkout workflow: a booking becomes **Completed** only after checkout is confirmed, not merely because its planned checkout date has arrived.
- Checkout creates an invoice and persists the checkout/invoice update as one database snapshot.
- Booking History displays completed stays for the selected time range, seven entries per page.
- Dashboard summary, room charts, and an A4 landscape PDF operational report.
- SQLite persistence with schema migration and transactional saves.

## Core behavior

### Booking lifecycle

```text
Upcoming ── check-in date reached ──► Active ── explicit checkout ──► Completed
    │
    └── cancel before check-in ──► Cancelled
```

- `cancelled` and `checkedOut` are persisted facts on a booking.
- `Upcoming` and `Active` are determined from the check-in date and current date.
- A completed booking is removed from the operational Reservations page and shown in Dashboard → Booking History.
- A room with an active guest cannot be reserved for a stay that covers today until the guest checks out.
- Maintenance is scheduled as a date interval, not a permanent room flag. A maintenance interval cannot overlap an active or upcoming booking, and booking validation rejects stays that overlap maintenance.
- A reservation being created or edited must have a checkout date after its check-in date. Restored historical same-day stays remain valid.

### Checkout and billing

1. Staff selects **Check-out** for an active booking.
2. `BookingService::completeBooking()` records the actual checkout date and sets `checkedOut`; `HotelManager` keeps the existing UI-facing facade.
3. An invoice is created for the completed stay.
4. `DataManager::commitChanges()` writes both changes atomically.

If persistence fails, the manager reloads the last committed database state rather than leaving memory and SQLite out of sync.

## Customer input rules

The customer and reservation dialogs share `src/views/CountryInputRules.h`.

- Supported countries: Vietnam, United States, Malaysia, United Kingdom, Japan, Singapore, South Korea, Thailand, Australia, and Germany.
- Each country has an ID pattern, compact field hint, calling code, and expected local phone length.
- Stored phone numbers use the country calling code plus normalized local number, for example `+84912345678`.

## Persistence

- Database file: `data/hotel_data.db`.
- SQLite schema: `Customer`, `Room`, `Booking`, `Invoice`, and `RoomMaintenance`.
- `Booking.checkedOut` is migrated automatically for legacy data. Historical records whose checkout date has passed, or that already have an invoice, are preserved as completed during the one-time migration.
- Loading is staged in a temporary `HotelManager`. If any persisted customer, room, booking, or invoice is invalid, loading fails without replacing the currently active in-memory state.
- Saving uses one SQLite transaction. `saveAll()` rejects broken object graphs and rolls back on failure.

## Dashboard and PDF report

The dashboard provides room and booking summary cards, charts, selected-range filtering, and completed-stay history.

The **Export Report** action creates an A4 landscape PDF with:

- reporting period and generated time;
- occupancy, room, and booking KPIs;
- room inventory and top rooms in the selected period;
- open booking activity, cancelled reservations, and completed booking history;
- guest, customer ID, phone, room, and stay dates where applicable.

PDF sections use print-safe grouping so a section heading is kept with its content; the Top Rooms and Completed Booking History blocks begin on a fresh page.

## Architecture

```text
src/
├── main.cpp
├── models/        Domain entities and room hierarchy
├── controllers/
│   ├── hotel/     HotelManager composition root and compatibility facade
│   ├── booking/   BookingManager and its booking/invoice/availability services
│   ├── customer/  CustomerManager and CustomerService
│   ├── room/      RoomManager and RoomService
│   └── report/    Read-only report aggregation
├── database/      DataManager SQLite load/save and migrations
└── views/         Qt widgets, dialogs, dashboard, and .ui files
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for layer boundaries, ownership, state, and persistence details.

## Build

### Requirements

- CMake 3.16+
- Qt 6: Core, Widgets, SQL, and Charts
- C++17 compiler compatible with the installed Qt kit

### CMake

```bash
cmake -S . -B build
cmake --build build --target HotelBookingManagement
```

Run the executable produced in the build directory. On first launch with an empty database, the application seeds demo rooms and persists them to `data/hotel_data.db`.

## Main classes

| Class | Responsibility |
|---|---|
| `Room`, `StandardRoom`, `DeluxeRoom`, `SuiteRoom`, `RoomMaintenance` | Polymorphic room hierarchy, pricing, and dated maintenance intervals |
| `Customer` | Guest identity and contact information |
| `Booking` | Reservation, cancellation flag, and explicit checkout flag |
| `Invoice` | Post-checkout billing information |
| `HotelManager` | In-memory entity collections, lookup/query facade, and compatibility API for views |
| `BookingManager` | Owns the immediate booking/invoice workflows for reservations and checkout |
| `CustomerManager` | Owns CustomerService for customer registration, matching, archive, and deletion flows |
| `RoomManager` | Owns RoomService for room registration, dated maintenance scheduling, archive, and deletion flows |
| `ReportService` | Builds a sorted Dashboard/PDF reporting snapshot from manager-owned domain data |
| `DataManager` | SQLite schema, migration, staged load, and transactional save |
| `RoomFactory` | Room object creation |
| `MainWindow` / page widgets | Qt desktop UI and navigation |

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
