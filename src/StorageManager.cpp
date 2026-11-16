#include <iostream>
#include <fstream>
#include <filesystem>
#include "StorageManager.h"

using namespace std;
namespace fs = filesystem;

bool StorageManager::createDatabase(const string& name) {

    fs::create_directories("data");

    string path = "data/" + name + ".db";

    if (fs::exists(path)) {
        return false;
    }

    ofstream file(path);

    if (!file.is_open()) {
        return false;
    }

    file << "CPPLITEDB\n";
    file << "VERSION 1\n";

    file.close();

    return true;
}

bool StorageManager::databaseExists(const string& name) {

    string path = "data/" + name + ".db";

    return fs::exists(path);
}

vector<string> StorageManager::getDatabases() const {

    vector<string> databases;

    if (!fs::exists("data")) {
        return databases;
    }

    for (const auto& entry : fs::directory_iterator("data")) {

        if (entry.is_regular_file() &&
            entry.path().extension() == ".db") {

            databases.push_back(entry.path().stem().string());
        }
    }

    return databases;
}