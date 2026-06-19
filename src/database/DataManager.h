#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <string>

class DataManager {
public:
    static DataManager& getInstance();

    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    bool loadAll(const std::string& dataPath = "data");
    bool saveAll(const std::string& dataPath = "data");

private:
    DataManager() = default;
    ~DataManager() = default;
};

#endif // DATAMANAGER_H
