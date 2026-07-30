#include "Customer.h"

Customer::Customer() {
    // Fixed-modified: Keep the default customer invalid until real data is assigned.
    this->customerId = "";
    this->documentType = "";
    this->issuingCountry = "";
    this->documentNumber = "";
    this->name = "";
    this->phoneNumber = "";
    this->archived = false;
}

Customer::Customer(const std::string& customerId, const std::string& name, const std::string& phoneNumber) {
    this->customerId = customerId;
    this->name = name;
    this->phoneNumber = phoneNumber;
    this->archived = false;
}

std::string Customer::getCustomerId() const {
    return this->customerId;
}

void Customer::setCustomerId(const std::string& customerId) {
    this->customerId = customerId;
}

std::string Customer::getDocumentType() const { return documentType; }
void Customer::setDocumentType(const std::string& value) { documentType = value; }
std::string Customer::getIssuingCountry() const { return issuingCountry; }
void Customer::setIssuingCountry(const std::string& value) { issuingCountry = value; }
std::string Customer::getDocumentNumber() const { return documentNumber.empty() ? customerId : documentNumber; }
void Customer::setDocumentNumber(const std::string& value) { documentNumber = value; }

std::string Customer::getName() const {
    return this->name;
}

void Customer::setName(const std::string& name) {
    this->name = name;
}

std::string Customer::getPhoneNumber() const {
    return this->phoneNumber;
}

void Customer::setPhoneNumber(const std::string& phoneNumber) {
    this->phoneNumber = phoneNumber;
}

bool Customer::isArchived() const {
    return archived;
}

void Customer::setArchived(bool archived) {
    this->archived = archived;
}
bool Customer::isValid() const {
    // Fixed-modified: Reject empty and placeholder customer records.
    return !customerId.empty() &&
           !documentType.empty() &&
           !issuingCountry.empty() &&
           !documentNumber.empty() &&
           !name.empty() &&
           !phoneNumber.empty() &&
           customerId != "UNKNOWN" &&
           name != "UNKNOWN" &&
           phoneNumber != "UNKNOWN";
}
