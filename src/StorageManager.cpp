#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
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

bool StorageManager::createTable(const string& database, const string& tableName, const vector<Column>& columns) {
    string path = "data/" + database + ".db";

    if (!fs::exists(path)) {
        return false;
    }

    // Check whether the table already exists
    ifstream input(path);
    string line;

    while (getline(input, line)) {
        if (line == "TABLE " + tableName) {
            input.close();
            return false;
        }
    }

    input.close();

    // Append the table schema to the database file
    ofstream output(path, ios::app);

    if (!output.is_open()) {
        return false;
    }

    output << "TABLE " << tableName << "\n";

    for (const auto& column : columns) {
        output << "COLUMN "
               << column.name << " "
               << column.type << "\n";
    }

    output << "END_TABLE\n";

    output.close();

    return true;
}
