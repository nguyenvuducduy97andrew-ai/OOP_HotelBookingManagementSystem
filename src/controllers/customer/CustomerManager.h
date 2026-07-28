#pragma once

#include "services/CustomerService.h"

class HotelManager;

class CustomerManager
{
public:
    // Modified and optimized performance: keep CustomerService as the manager's only direct workflow dependency.
    explicit CustomerManager(HotelManager& hotelManager);
    bool registerCustomer(const std::string& id, const std::string& name, const std::string& phone,
                          std::string& errorMessage, std::string* conflictingCustomerId = nullptr);
    // Modified: Expose validated customer edits through the direct CustomerManager boundary.
    bool updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone,
                        std::string& errorMessage, std::string* conflictingCustomerId = nullptr);
    bool resolveForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
    bool archiveCustomer(const std::string& customerId, std::string& errorMessage);
    bool restoreCustomer(const std::string& customerId, std::string& errorMessage);
    bool deleteCustomer(const std::string& customerId, std::string& errorMessage);
private:
    CustomerService m_customerService;
};
