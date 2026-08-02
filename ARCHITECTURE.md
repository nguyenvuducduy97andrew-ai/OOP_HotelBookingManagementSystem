# Architecture

## Purpose

This document describes the controller architecture after the 2026 ownership refactor. The refactor preserved the public `HotelManager` API, business behavior, database schema, and view workflows while removing the temporary manager/service callback structure.

## Controller structure

```text
Views --------------------+
ReportService ------------+--> HotelManager
DataManager --------------+         |
                                    +--> RoomManager
                                    +--> CustomerManager
                                    `--> BookingManager
```

The dependency direction is one-way. Views, reporting, and persistence use `HotelManager`; domain managers never call back into it.

```text
src/controllers/
  hotel/
    HotelManager.h
    HotelManager.cpp
  room/
    RoomManager.h
    RoomManager.cpp
  customer/
    CustomerManager.h
    CustomerManager.cpp
  booking/
    BookingManager.h
    BookingManager.cpp
  report/
    ReportService.h
    ReportService.cpp
```

There are no controller service forwarding layers, service directories, `*Core` callbacks, or controller friend declarations.

## Ownership

`HotelManager` owns one persistent instance of each domain manager by value:

```text
HotelManager
  |- RoomManager
  |    |- vector<shared_ptr<Room>>
  |    |- vector<RoomMaintenance>
  |    `- vector<MaintenanceGuestNotice>
  |- CustomerManager
  |    `- vector<shared_ptr<Customer>>
  `- BookingManager
       |- vector<shared_ptr<Booking>>
       `- vector<shared_ptr<Invoice>>
```

Each collection has one canonical owner. `HotelManager` does not keep duplicate entity collections.

Bookings hold weak links to their canonical customer and room. Invoices hold a weak link to their canonical booking and also retain immutable billing snapshots. Weak links avoid ownership cycles while manager-owned `shared_ptr` collections keep canonical entities alive.

## Responsibilities

### HotelManager

`HotelManager` is the stable application facade and cross-domain coordinator.

It:

- preserves the API consumed by views, `ReportService`, `DataManager`, and `main.cpp`;
- forwards single-domain commands and queries to the owning manager;
- resolves canonical customers and rooms before booking creation, update, or restoration;
- coordinates workflows involving more than one domain;
- exposes persistence restoration operations without exposing manager internals;
- owns managers by value so staged move-assignment remains safe.

It does not contain domain collections, detailed service implementations, manager accessors, or callback APIs.

### RoomManager

`RoomManager` owns rooms, maintenance cases, and maintenance guest notices. It handles room validation, registration, lookup, permanent availability, archive/restore, local deletion, maintenance interval validation, maintenance-to-maintenance conflicts, notice storage, and database restoration.

It does not depend on `HotelManager` or `BookingManager`.

### CustomerManager

`CustomerManager` owns customers. It handles identity validation, normalized collision rules, registration, update, booking-customer resolution, archive/restore, local deletion, lookup, and database restoration.

It does not depend on `HotelManager` or `BookingManager`.

### BookingManager

`BookingManager` owns bookings and invoices. It handles:

- booking validation and lifecycle transitions;
- booking state and date/status queries;
- overlap and excluded-booking rules;
- date-range availability using room and maintenance values supplied per call;
- invoice creation, eligibility, snapshots, calculations, lookup, and ID generation;
- booking and invoice restoration.

It stores no reference to another manager or to `HotelManager`.

### ReportService

`ReportService` stores a `const HotelManager*` and performs read-only aggregation. It builds `DashboardReportData`, including occupancy, booking status, room demand, completed stays, cancellation/no-show details, and invoice-based revenue metrics. It does not receive mutable manager access.

### DataManager

`DataManager` remains the SQLite persistence boundary. It calls only the `HotelManager` facade and does not own model objects.

## Cross-domain workflows

### Create or update a booking

```text
View
  -> HotelManager
       |- resolve Customer through CustomerManager
       |- resolve Room through RoomManager
       `- BookingManager validates dates, capacity, maintenance, and overlap
            -> store Booking with canonical Customer and Room pointers
```

Update availability passes the current booking ID as an exclusion so a booking does not conflict with itself.

### Availability

`BookingManager` is the single date-range availability authority. `HotelManager` supplies read-only room and maintenance collections for each call. Reservation and Room Status use the `HotelManager` facade, so they cannot disagree by bypassing the booking rules.

Cancelled, deleted, completed, and no-show records do not block future stays. Active stays and unfinished overlapping bookings block according to the existing half-open interval rule:

```text
requestedCheckIn < existingCheckOut
&& existingCheckIn < requestedCheckOut
```

Maintenance intervals use the same `[start, end)` semantics.

### Maintenance

`RoomManager` validates and stores maintenance-local data. `HotelManager` asks `BookingManager` data which unfinished bookings overlap, decides whether the case is confirmed or awaiting guest response, and coordinates confirmation after conflicts are resolved. This prevents a RoomManager/BookingManager dependency cycle.

### Room and customer removal

`HotelManager` checks booking history before asking the owning manager to delete a room or customer. Archive operations similarly reject entities referenced by active or upcoming bookings.

### Invoice creation

`BookingManager` validates that checkout is complete, enforces one invoice per booking, derives nights and locked pricing/tax, captures immutable customer/room/stay snapshots, and stores the invoice. Checkout and invoice creation remain separate calls because combining them would change the workflow.

## Booking state

Booking state is derived from persisted facts:

1. cancellation with a `No-show:` reason -> `NO_SHOW`;
2. other cancellation -> `CANCELLED`;
3. checked out -> `COMPLETED`;
4. checked in -> `ACTIVE`;
5. otherwise -> `UPCOMING`.

The database does not store a separate authoritative state column.

## Persistence and move safety

`DataManager::loadAll()` restores in this order:

```text
Customer -> Room -> Booking -> RoomMaintenance
         -> MaintenanceGuestNotice -> Invoice
```

Loading is staged:

```cpp
HotelManager loadedManager;
// restore and validate every row
manager = std::move(loadedManager);
```

`HotelManager` and every manager contain value-owned collections and no cross-manager references. Compile-time assertions require `HotelManager` to remain move-constructible and move-assignable.

Before publication, DataManager validates the complete object graph:

- every booking points to the canonical customer and room;
- every invoice points to the canonical booking;
- every maintenance case refers to an existing room;
- every maintenance notice refers to an existing booking.

Any invalid row or graph aborts loading before live state is replaced. Save operations retain revision checking, a single SQLite transaction, rollback on failure, and restoration of the last committed snapshot through `commitChanges()`.

## View and report boundary

Views store or receive `HotelManager*`; none includes a domain manager. No public mutable manager getter exists. `ReportService` is read-only.

`StatisticsPageWidget` is a read-only visualization and invoice-exploration view. It reads bookings and invoices through the `HotelManager` facade, builds its chart series and filtered table locally, and opens `InvoiceDialog` for invoice details. Booking changes and completed checkout events trigger `refreshData()` so its visualizations remain synchronized with the shared manager state.


## Dependency rules

| Dependency | Allowed |
|---|---|
| Views -> `HotelManager` | Yes |
| Views -> domain managers | No |
| `ReportService` -> const `HotelManager` queries | Yes |
| `DataManager` -> `HotelManager` persistence facade | Yes |
| `HotelManager` -> owned managers | Yes |
| Manager -> `HotelManager` | No |
| RoomManager/CustomerManager -> BookingManager | No |
| BookingManager -> stored manager reference | No |
| Models -> views/controllers/persistence | No |

## Design patterns

- **Facade:** `HotelManager` is the single application-facing controller boundary.
- **Composition:** `HotelManager` owns persistent domain managers by value.
- **Factory:** `RoomFactory` creates concrete room subtypes.
- **Singleton:** `DataManager` and `StaffSession` provide process-level contexts.
- **Staged publication:** DataManager constructs and validates a full aggregate before move-assignment.
- **Immutable snapshot:** invoices preserve historical billing inputs independently of later customer or room edits.
