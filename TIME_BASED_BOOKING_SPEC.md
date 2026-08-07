# Time-Based Booking Conversion Specification

> Status: implemented operational baseline. This document defines the authoritative time-based rules represented by the current application. Future service-line expansion remains explicitly identified as future scope.

## 1. Purpose and scope

The application is changing from a date/night reservation model to a time-based room-operations model. Reservations, availability, cleaning, maintenance, checkout, invoices, and operational reports must therefore use planned and actual time facts consistently.

The target system supports:

- planned reservations in one-hour slots;
- actual check-in and checkout timestamps recorded to second precision;
- availability by room time interval;
- automatic post-checkout cleaning;
- hourly billing from the actual stay duration;
- early check-in when the room is explicitly ready; and
- full-day maintenance windows.

This specification is the source of truth when an older document, screen, or implementation still refers to date-only reservations or nightly billing.

## 2. Terms

| Term | Meaning |
|---|---|
| Planned check-in / checkout | The schedule selected for a reservation. It protects future capacity and is shown in booking screens and operational reports. |
| Actual check-in / checkout | The moment staff performs the operational action. It is recorded with second precision and determines billing duration. |
| Blocking interval | A time range during which a room cannot be sold. Bookings, Cleaning, Maintenance, and applicable soft holds can block a room. |
| Cleaning | An automatically created, temporary room-unavailable interval after actual checkout. |
| Room ready | A staff-confirmed release of an active Cleaning interval before its expected end. |
| Billable hours | Actual stay duration rounded to the nearest whole hour, with a minimum of one hour. |

All time values belong to the hotel's local operating timezone. The application must use a consistent timezone for persistence, comparison, display, and reporting.

## 3. Time interval convention

Every interval uses a half-open boundary:

```text
[start, end)
```

The start is included and the end is excluded. Therefore a new interval may begin exactly when the previous blocking interval ends.

Example:

```text
Cleaning: 10:00 -> 12:00
New planned check-in at 12:00: valid
New planned check-in at 11:59: invalid
```

An interval is invalid when its end is not strictly after its start.

## 4. Reservation schedule and availability

### 4.1 Planned schedule

Staff select planned check-in and checkout using one-hour slots from `00:00` through `23:00`. The shortest planned reservation is one hour. A checkout at the following midnight must be displayed unambiguously as `00:00 next day`.

The schedule picker has a `Check-in | Check-out` mode selector, a compact seven-day calendar, and a 24-slot hour grid arranged as six columns by four rows. Week numbers are internal calendar metadata and are not displayed. The calendar may show a day as partially available, but the hour grid is authoritative for the final selected interval.

Endpoint selection is mode-driven. While `Check-in` is active, every valid date or hour click replaces the prior planned arrival and resets the tentative checkout to exactly one hour later. It must not preserve an older checkout and silently create a long range. Staff explicitly switches to `Check-out` before selecting the departure endpoint. In checkout-only extension mode, actual check-in remains fixed and only a later planned checkout is editable.

### 4.2 Calendar states

| State | Behaviour |
|---|---|
| Available | A usable continuous interval remains. |
| Limited | Only some usable hour slots remain. The date may still be selected. |
| Unavailable | No usable interval remains. The date and relevant hour slots are disabled, grey, non-hoverable, and non-clickable. |

Past dates are unavailable. A date is not disabled merely because part of that day is blocked; it remains selectable if at least one continuous hour can still be booked. When a selection crosses midnight, availability must remain continuous into the next date.

### 4.3 Final validation

The user interface is only a convenience. Before a reservation is created, edited, moved to another room, or checked in early, the backend must re-evaluate current availability. The reservation being edited is excluded from its own overlap check.

The final availability check considers unfinished bookings, active Cleaning, confirmed Maintenance, and applicable soft holds.

## 5. Cleaning and maintenance

### 5.1 Cleaning

Actual checkout immediately creates a Cleaning interval for the same room:

```text
cleaningStart = actualCheckoutAt
cleaningExpectedEnd = actualCheckoutAt + 2 hours
```

Cleaning is visually presented like Maintenance because both make a room unavailable. They remain distinct event types for history and reporting.

Reception may mark the room ready before the expected end after confirming that cleaning/preparation is complete. The system records the actor and actual completion time. If nobody releases it early, Cleaning ends automatically at the expected end.

Cleaning cannot make a room available while another blocking interval, such as Maintenance, remains active.

### 5.2 Planned turnover protection

When accepting a future booking, the system reserves a standard two-hour preparation window after the preceding booking's planned checkout:

```text
next planned check-in >= prior planned checkout + 2 hours
```

This protects the schedule without preventing an earlier actual check-in when the prior guest has checked out and the room is explicitly ready.

### 5.3 Maintenance

Maintenance retains its existing confirmation and affected-upcoming-booking workflow. It is a full-day operation:

```text
Maintenance from Thursday through Friday
= [Thursday 00:00, Saturday 00:00)
```

All hour slots on both named dates are unavailable. Maintenance remains distinct from Cleaning in data, audit history, and reporting.

## 6. Booking lifecycle

```text
Upcoming -- actual check-in --> Active -- checkout / early checkout --> Completed
    |
    `-- cancel --> Cancelled
```

- An Upcoming reservation becomes Active only through an actual check-in action.
- An Active reservation cannot be cancelled.
- An Active reservation may use `Check out` or `Early check-out`; both record actual checkout time and then start Cleaning.
- The former no-show action is merged into the Cancelled workflow. A cancellation reason may record that the guest did not arrive.
- Cancelled reservations do not create Cleaning because the guest did not occupy the room.

## 7. Actual-duration billing

### 7.1 Duration and rounding

Billing uses actual timestamps, not planned timestamps.

```text
actualDurationSeconds = actualCheckoutAt - actualCheckInAt
durationHours = actualDurationSeconds / 3600
billableHours = max(1, round-half-up(durationHours))
```

At an exact 30-minute fractional boundary, rounding is up. Examples:

| Actual stay | Billable hours |
|---|---:|
| 08:00 -> 10:25 | 2 |
| 08:00 -> 10:30 | 3 |
| 08:00 -> 10:31 | 3 |
| 08:00 -> 11:01 | 3 |
| 08:00 -> 08:20 | 1 |

There is no separate late-checkout fee. A late checkout changes the actual duration and therefore the rounded billable hours. Planned checkout remains an operational schedule reference and may trigger a staff warning when a later arrival is at risk.

An early actual check-in also starts chargeable time immediately. An early actual checkout uses the actual duration, subject to the one-hour minimum.

### 7.2 Rates and service lines

Each room type has a fixed hourly base rate:

```text
Standard hourly rate
Deluxe hourly rate
Suite hourly rate
```

The booking snapshots its quoted hourly room rate. Future optional services are hourly line items:

```text
hourlyTotal = quotedRoomHourlyRate
            + sum(serviceHourlyRate × selectedQuantity)

preTaxTotal = hourlyTotal × billableHours
```

Every selected service line must snapshot its name, hourly rate, and quantity so later price changes never alter historical invoices.

## 8. Operational warnings

The system must present clear internal warnings for:

- a requested interval that overlaps a booking, Cleaning, Maintenance, or hold;
- actual checkout that leaves less than the standard two-hour preparation window before a future arrival;
- an expected Cleaning completion later than a future planned arrival; and
- an early-arriving guest whose room is not ready.

The warning identifies the room, relevant time, and affected booking. It supports reception judgement; a manager-override workflow is not required in the current project scope.

## 9. Booking dialog workflow

Reservation creation begins after an available room is reviewed from Room Status.

```text
Selected Room Status card
  -> Read-only Room Info
  -> Booking action
  -> Existing customer | New customer
  -> Stay schedule
  -> Adults / Children / Extra beds
  -> Review room and booking
  -> Create reservation
```

For an existing customer, search matches name, document details, and phone. The selected customer's identity and contact data are read-only in the reservation. Only booking-specific fields are editable.

For a new customer, identity and contact fields are collected and validated. Customer creation and reservation creation must succeed or fail together, so cancelling an unfinished reservation does not leave an orphan customer record.

The selected room is reviewed first in `RoomInfoDialog`, containing room type, capacity, bed configuration, hourly rate, amenities, and status. Closing the review cancels navigation; only its Booking action opens Reservation. Choosing another room in Reservation is a one-click selection that refreshes the review and quote, and final save revalidates availability.

## 10. Reports and historical data

Operational reports distinguish planned and actual facts:

- planned arrivals, departures, and schedule conflicts use planned times;
- actual occupancy, Cleaning, checkout, invoice revenue, and billable hours use actual facts.

Future capacity KPIs use room-hours rather than only room-nights. Cleaning and Maintenance reduce saleable availability but are not occupied guest hours.

Existing date-only bookings and invoices are legacy records. Migration must preserve their stored financial totals and must not invent actual timestamps or recalculate historical invoices from defaults.

## 11. Implementation safeguards

The conversion must:

- centralize interval, billing-rounding, and room-readiness rules rather than duplicate them in views;
- use integer seconds for duration and integer VND-compatible monetary values where practical;
- snapshot commercial rates at booking/invoice time;
- persist audit facts for early Cleaning completion;
- validate availability again at final save/check-in; and
- migrate existing data transactionally without silently deleting or altering historical financial facts.

Non-obvious implementation rules must be documented in source with English comments in this form:

```cpp
// Modified: <main business rule or technical reason>
```

## 12. Acceptance scenarios

Before the conversion is accepted, the project must demonstrate:

1. A booking can start exactly when a prior Cleaning interval ends.
2. A conflicting interval is rejected both in the picker and at final save.
3. A room enters Cleaning at actual checkout, can be released early by reception, and releases automatically after two hours.
4. Maintenance from Thursday to Friday blocks all of both days.
5. Actual-duration rounding is correct at 25, 30, 31, 61, and sub-30-minute cases.
6. An early-ready room allows early actual check-in and begins billing at that timestamp.
7. An Active booking offers checkout actions rather than cancellation.
8. A Cancelled booking has no Cleaning interval.
9. A quoted room/service rate remains unchanged after the price configuration changes.
10. Legacy invoices retain their historical totals after database migration.
