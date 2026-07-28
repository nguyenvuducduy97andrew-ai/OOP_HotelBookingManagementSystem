#include "CustomerService.h"
#include "../../hotel/HotelManager.h"

#include <QString>

namespace {
QString normalizedName(const std::string& name)
{
    return QString::fromStdString(name).simplified();
}
}

CustomerService::CustomerService(HotelManager& hotelManager) : m_hotelManager(hotelManager) {}

bool CustomerService::registerCustomer(const std::string& id, const std::string& name, const std::string& phone,
                                      std::string& errorMessage, std::string* conflictingCustomerId)
{
    if (conflictingCustomerId) {
        conflictingCustomerId->clear();
    }

    const QString inputName = normalizedName(name);
    std::shared_ptr<Customer> phoneMatch;
    std::shared_ptr<Customer> idMatch;

    for (const auto& existing : m_hotelManager.getCustomers()) {
        if (!existing) {
            continue;
        }
        if (!phoneMatch && existing->getPhoneNumber() == phone) {
            phoneMatch = existing;
        }
        if (!idMatch && existing->getCustomerId() == id) {
            idMatch = existing;
        }
    }

    const auto reportConflict = [&conflictingCustomerId](const std::shared_ptr<Customer>& customer) {
        if (conflictingCustomerId && customer) {
            *conflictingCustomerId = customer->getCustomerId();
        }
    };

    // Modified and optimized performance: enforce the agreed phone/name/ID collision order before a new customer reaches the shared collection.
    if (phoneMatch) {
        const bool sameName = normalizedName(phoneMatch->getName()).compare(inputName, Qt::CaseInsensitive) == 0;
        if (sameName && phoneMatch->getCustomerId() == id) {
            reportConflict(phoneMatch);
            errorMessage = "This customer already exists.";
            return false;
        }

        reportConflict(phoneMatch);
        errorMessage = "This phone number is already used by another customer. Please use a different number.";
        return false;
    }

    if (idMatch) {
        reportConflict(idMatch);
        errorMessage = "This ID number is already linked to another customer account. Please verify and enter it again.";
        return false;
    }

    return m_hotelManager.registerCustomerCore(id, name, phone, errorMessage);
}

bool CustomerService::resolveForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage) { return m_hotelManager.resolveCustomerForBookingCore(id, name, phone, errorMessage); }
bool CustomerService::archiveCustomer(const std::string& customerId, std::string& errorMessage) { return m_hotelManager.archiveCustomerCore(customerId, errorMessage); }
bool CustomerService::restoreCustomer(const std::string& customerId, std::string& errorMessage) { return m_hotelManager.restoreCustomerCore(customerId, errorMessage); }
bool CustomerService::deleteCustomer(const std::string& customerId, std::string& errorMessage) { return m_hotelManager.deleteCustomerCore(customerId, errorMessage); }
