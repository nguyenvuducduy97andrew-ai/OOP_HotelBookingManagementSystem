#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Customer.h"

class CustomerManager
{
public:
    CustomerManager();

    const std::vector<std::shared_ptr<Customer>>& getCustomers() const;
    std::shared_ptr<Customer> findCustomerById(const std::string& customerId) const;
    bool customerIdExists(const std::string& customerId) const;

    bool registerCustomer(const std::string& id, const std::string& name, const std::string& phone,
                          std::string& errorMessage, std::string* conflictingCustomerId = nullptr);
    bool updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone,
                        std::string& errorMessage, std::string* conflictingCustomerId = nullptr);
    bool resolveForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage);
    bool archiveCustomer(const std::string& customerId, std::string& errorMessage);
    bool restoreCustomer(const std::string& customerId, std::string& errorMessage);
    bool deleteCustomer(const std::string& customerId, std::string& errorMessage);
    bool restoreCustomerFromDatabase(const std::string& customerId,
                                     const std::string& documentType,
                                     const std::string& issuingCountry,
                                     const std::string& documentNumber,
                                     const std::string& name,
                                     const std::string& phone,
                                     bool archived,
                                     std::string& errorMessage);

    void clearAll();

private:
    std::vector<std::shared_ptr<Customer>> m_customers;
};
