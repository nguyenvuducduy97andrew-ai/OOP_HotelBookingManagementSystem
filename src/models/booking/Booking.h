#pragma once
#include <string>
#include <memory>

class Customer;
class Room;

enum class BookingState { UPCOMING, ACTIVE, COMPLETED, CANCELLED, NO_SHOW };

class Booking
{
private:
    static int bookingCounter;
    std::string bookingId;
    std::weak_ptr<Customer> customer;  // Weak reference to customer (owned by HotelManager)
    std::weak_ptr<Room> room;          // Weak reference to room (owned by HotelManager)
    std::string checkInDate;
    std::string checkOutDate;
    // Modified: Preserve planned datetime facts separately from legacy date-only fields during the staged time-based migration.
    std::string plannedCheckInAt;
    std::string plannedCheckOutAt;
    bool cancelled; 
    bool deleted;
    // Modified: Persist explicit arrival and departure facts separately from the reservation's planned dates.
    bool checkedIn;
    bool checkedOut;
    std::string actualCheckInDate;
    std::string actualCheckOutDate;
    // Modified: Store precise operational timestamps without inventing times for legacy records.
    std::string actualCheckInAt;
    std::string actualCheckOutAt;
    // Modified: Lock the confirmed room rate and tax at reservation time for stable billing.
    double quotedUnitPrice;
    // Modified: Keep the future hourly quote independent from legacy nightly pricing until hourly billing is activated.
    double quotedHourlyRate;
    bool legacyDateOnlySchedule;
    double quotedTaxRate;
    // Modified: Persist reservation occupancy so capacity validation is enforceable beyond the UI.
    int adultCount;
    int childCount;
    // Modified: Retain cancellation context for operational audit instead of removing the reservation.
    std::string cancellationReason;
    std::string cancelledAt;
    // Modified: Persist creation and last-change timestamps so booking history can be audited without inferring events from planned dates.
    std::string createdAt;
    std::string updatedAt;

public:
    Booking();

    // Modified and optimized performance: restrict booking-ID assignment to the manager facade and controlled booking workflow.
    friend class BookingManager;

    static std::string nextBookingId();
    static void initCounterFromDatabase(int maxBookingNumber);

    std::string getBookingId() const;
    
private:
    // Fixed-modified: Keep booking IDs write-protected outside the manager layer.
    void setBookingId(const std::string &bookingId);

public:

    std::shared_ptr<Customer> getCustomer() const;
    void setCustomer(const std::shared_ptr<Customer> &customer);

    std::shared_ptr<Room> getRoom() const;
    void setRoom(const std::shared_ptr<Room> &room);

    std::string getCheckInDate() const;
    void setCheckInDate(const std::string &checkInDate);

    std::string getCheckOutDate() const;
    void setCheckOutDate(const std::string &checkOutDate);

    std::string getPlannedCheckInAt() const;
    void setPlannedCheckInAt(const std::string& value);
    std::string getPlannedCheckOutAt() const;
    void setPlannedCheckOutAt(const std::string& value);

    bool isCancelled() const;
    void setCancelled(bool cancelled);

    bool isDeleted() const;
    void setDeleted(bool deleted);

    // Modified and optimized performance: expose the persisted checkout state used by operational and history views.
    bool isCheckedOut() const;
    void setCheckedOut(bool checkedOut);

    bool isCheckedIn() const;
    void setCheckedIn(bool checkedIn);

    std::string getActualCheckInDate() const;
    void setActualCheckInDate(const std::string& value);
    std::string getActualCheckOutDate() const;
    void setActualCheckOutDate(const std::string& value);
    std::string getActualCheckInAt() const;
    void setActualCheckInAt(const std::string& value);
    std::string getActualCheckOutAt() const;
    void setActualCheckOutAt(const std::string& value);
    std::string getEffectiveCheckOutDate() const;

    double getQuotedUnitPrice() const;
    void setQuotedUnitPrice(double value);
    double getQuotedHourlyRate() const;
    void setQuotedHourlyRate(double value);
    bool usesLegacyDateOnlySchedule() const;
    void setLegacyDateOnlySchedule(bool value);
    double getQuotedTaxRate() const;
    void setQuotedTaxRate(double value);

    int getAdultCount() const;
    void setAdultCount(int value);
    int getChildCount() const;
    void setChildCount(int value);

    std::string getCancellationReason() const;
    void setCancellationReason(const std::string& value);
    std::string getCancelledAt() const;
    void setCancelledAt(const std::string& value);
    std::string getCreatedAt() const;
    void setCreatedAt(const std::string& value);
    std::string getUpdatedAt() const;
    void setUpdatedAt(const std::string& value);

    bool isValid() const;  // Check if booking has valid (non-expired) customer and room
};
