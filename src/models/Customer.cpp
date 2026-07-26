#include "Customer.h"

Customer::Customer() {
    this->customerId = "UNKNOWN";
    this->name = "UNKNOWN";
    this->phoneNumber = "UNKNOWN";
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
    return !customerId.empty() && !name.empty() && !phoneNumber.empty();
}