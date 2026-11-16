#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <vector>

class StorageManager {
public:
    bool createDatabase(const std::string& name);
    bool databaseExists(const std::string& name);
    std::vector<std::string> getDatabases() const;
};

#endif