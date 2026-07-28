#pragma once

#include <string>

class HotelManager;

class CustomerService
{
public:
    explicit CustomerService(HotelManager& hotelManager);
    bool registerCustomer(const std::string& id, const std::string& name, const std::string& phone,
                          std::string& errorMessage, std::string* conflictingCustomerId = nullptr);
    // Modified: Validate customer edits with the same duplicate-detection policy as customer creation.
    bool updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone,
                        std::string& errorMessage, std::string* conflictingCustomerId = nullptr);
    bool resolveForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
    bool archiveCustomer(const std::string& customerId, std::string& errorMessage);
    bool restoreCustomer(const std::string& customerId, std::string& errorMessage);
    bool deleteCustomer(const std::string& customerId, std::string& errorMessage);

private:
    HotelManager& m_hotelManager;
};
