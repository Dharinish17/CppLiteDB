#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>

class StorageManager {
public:
    bool createDatabase(const std::string& name);
    bool databaseExists(const std::string& name);
};

#endif