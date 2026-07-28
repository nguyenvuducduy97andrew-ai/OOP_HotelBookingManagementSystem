#include "CustomerManager.h"
#include "../hotel/HotelManager.h"

CustomerManager::CustomerManager(HotelManager& hotelManager) : m_customerService(hotelManager) {}
bool CustomerManager::registerCustomer(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage, std::string* conflictingCustomerId) { return m_customerService.registerCustomer(id, name, phone, errorMessage, conflictingCustomerId); }
bool CustomerManager::updateCustomer(const std::string& customerId, const std::string& name, const std::string& phone, std::string& errorMessage, std::string* conflictingCustomerId) { return m_customerService.updateCustomer(customerId, name, phone, errorMessage, conflictingCustomerId); }
bool CustomerManager::resolveForBooking(const std::string& id, const std::string& name, const std::string& phone, std::string& errorMessage) { return m_customerService.resolveForBooking(id, name, phone, errorMessage); }
bool CustomerManager::archiveCustomer(const std::string& customerId, std::string& errorMessage) { return m_customerService.archiveCustomer(customerId, errorMessage); }
bool CustomerManager::restoreCustomer(const std::string& customerId, std::string& errorMessage) { return m_customerService.restoreCustomer(customerId, errorMessage); }
bool CustomerManager::deleteCustomer(const std::string& customerId, std::string& errorMessage) { return m_customerService.deleteCustomer(customerId, errorMessage); }
