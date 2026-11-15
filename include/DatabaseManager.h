#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include "StorageManager.h"

class DatabaseManager {
private:
    StorageManager storage;
    std::string activeDatabase;

public:
    void start();
};

#endif