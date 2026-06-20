#pragma once

#include <string>

// Central entry point for loading and saving hotel data.
// This class is implemented as a singleton so the application uses one
// shared data manager for all operations.
class DataManager {
    private:
    DataManager() = default;
    ~DataManager() = default;
public:
    // Returns the single shared DataManager instance.
    static DataManager& getInstance();

    // Copying is disabled to preserve the singleton guarantee. 
    //=> Only ONE object of this class exist in the system
    //Assignment is disabled for the same reason.
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    bool loadAll(const std::string& dataPath = "data");
    bool saveAll(const std::string& dataPath = "data");

};


