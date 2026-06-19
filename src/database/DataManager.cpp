#include "DataManager.h"

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

bool DataManager::loadAll(const std::string& dataPath) {
    // TODO: load rooms, customers, and bookings from JSON/CSV/SQLite
    (void)dataPath;
    return true;
}

bool DataManager::saveAll(const std::string& dataPath) {
    // TODO: persist rooms, customers, and bookings
    (void)dataPath;
    return true;
}
