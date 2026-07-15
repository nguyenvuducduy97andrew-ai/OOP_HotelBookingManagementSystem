#include "Customer.h"

Customer::Customer() {
    this->customerId = "";
    this->name = "";
    this->phoneNumber = "";
    this->archived = false;
}

Customer::Customer(std::string customerId, std::string name, std::string phoneNumber) {
    this->customerId = customerId;
    this->name = name;
    this->phoneNumber = phoneNumber;
    this->archived = false;
}

std::string Customer::getCustomerId() const {
    return this->customerId;
}

void Customer::setCustomerId(std::string customerId) {
    this->customerId = customerId;
}

std::string Customer::getName() const {
    return this->name;
}

void Customer::setName(std::string name) {
    this->name = name;
}

std::string Customer::getPhoneNumber() const {
    return this->phoneNumber;
}

void Customer::setPhoneNumber(std::string phoneNumber) {
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