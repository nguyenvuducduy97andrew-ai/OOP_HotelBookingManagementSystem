#include "DataManager.h"

// Creates or returns the one shared DataManager instance.
DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

// Loads application data from the configured storage location.
bool DataManager::loadAll(const std::string& dataPath) {// the data path
    // TODO: load rooms, customers, and bookings from JSON/CSV/SQLite
    (void)dataPath;
    return true;
}

// Saves application data to the configured storage location.
bool DataManager::saveAll(const std::string& dataPath) {
    // TODO: persist rooms, customers, and bookings
    (void)dataPath;
    return true;
}
