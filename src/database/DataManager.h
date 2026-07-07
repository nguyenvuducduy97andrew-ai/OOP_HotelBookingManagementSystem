#pragma once
#include "HotelManager.h"
#include <string>
#include <QSqlDatabase>

// Central entry point for loading and saving hotel data.
// This class is implemented as a singleton so the application uses one
// shared data manager for all operations.
class DataManager {
private:
    DataManager() = default;
    ~DataManager() = default;

    // Helper function to set up the connection and create the initial structure tables
    bool initDatabase(const std::string& dataPath);

    QSqlDatabase m_db; // Qt database connection storage variable

public:
    // Returns the single shared DataManager instance.
    static DataManager& getInstance();

    // Copying is disabled to preserve the singleton guarantee.
    //=> Only ONE object of this class exist in the system
    //Assignment is disabled for the same reason.
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    // Modified: Updated documentation to reflect that loading now populates the Booking lifecycle status
    bool loadAll(HotelManager& manager, const std::string& dataPath = "hotel_data.db");

    // Modified: Removed 'const' from HotelManager reference to allow dynamic calculations during serialization
    // and to ensure the new Booking lifecycle state is correctly saved back to the database
    bool saveAll(HotelManager& manager, const std::string& dataPath = "hotel_data.db");
};