#pragma once
#include <string>

class Customer {
private:
    std::string customerId;
    std::string documentType;
    std::string issuingCountry;
    std::string documentNumber;
    std::string name;
    std::string phoneNumber;
    bool archived = false;
    // QList<Booking*> bookingHistory;
public:
    Customer();
    Customer(const std::string& customerId, const std::string& name, const std::string& phoneNumber);
    // A virtual destructor is unnecessary without inheritance. Restore it if
    // specialised customer types (for example, VIPCustomer) are introduced.

    std::string getCustomerId() const;
    void setCustomerId(const std::string& customerId);

    std::string getDocumentType() const;
    void setDocumentType(const std::string& documentType);
    std::string getIssuingCountry() const;
    void setIssuingCountry(const std::string& issuingCountry);
    std::string getDocumentNumber() const;
    void setDocumentNumber(const std::string& documentNumber);

    std::string getName() const;
    void setName(const std::string& name);

    std::string getPhoneNumber() const;
    void setPhoneNumber(const std::string& phoneNumber);

    bool isArchived() const;
    void setArchived(bool archived);

    bool isValid() const;
};
