#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
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

//SHOW table
vector<string> StorageManager::getTables(const string& database) const {

    vector<string> tables;

    string path = "data/" + database + ".db";

    ifstream file(path);

    if (!file.is_open()) {
        return tables;
    }

    string line;

    while (getline(file, line)) {

        if (line.find("TABLE ") == 0) {

            string tableName = line.substr(6);

            tables.push_back(tableName);
        }
    }

    file.close();

    return tables;
}

//DESCRIBE table
vector<Column> StorageManager::getTableColumns(
    const string& database,
    const string& tableName
) const {

    vector<Column> columns;

    string path = "data/" + database + ".db";

    ifstream file(path);

    if (!file.is_open()) {
        return columns;
    }

    string line;
    bool tableFound = false;

    while (getline(file, line)) {

        // Check whether this is the requested table
        if (line == "TABLE " + tableName) {
            tableFound = true;
            continue;
        }

        // Stop reading when the table definition ends
        if (tableFound && line == "END_TABLE") {
            break;
        }

        // Extract column information
        if (tableFound && line.find("COLUMN ") == 0) {

            stringstream ss(line.substr(7));

            Column column;

            if (ss >> column.name >> column.type) {
                columns.push_back(column);
            }
        }
    }

    file.close();

    return columns;
}


bool StorageManager::insertRow(
    const string& database,
    const string& tableName,
    const vector<string>& values
) {
    string path = "data/" + database + ".db";

    ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    vector<string> lines;
    string line;
    bool tableFound = false;
    int dataStart = -1;
    int dataEnd = -1;

    // Read the existing database file.
    while (getline(input, line)) {
        if (line == "TABLE " + tableName) {
            tableFound = true;
        }

        if (line == "DATA " + tableName) {
            dataStart = static_cast<int>(lines.size());
        }

        if (line == "END_DATA" && dataStart != -1) {
            dataEnd = static_cast<int>(lines.size());
        }

        lines.push_back(line);
    }

    input.close();

    if (!tableFound) {
        return false;
    }

    // Prepare the row.
    string row;

    for (size_t i = 0; i < values.size(); i++) {
        if (values[i].find('|') != string::npos ||
            values[i].find('\n') != string::npos) {
            return false;
        }

        if (i > 0) {
            row += "|";
        }

        row += values[i];
    }

    // Add a new DATA section if one doesn't exist.
    if (dataStart == -1) {
        lines.push_back("DATA " + tableName);
        lines.push_back(row);
        lines.push_back("END_DATA");
    }
    else {
        // Insert the row before END_DATA.
        lines.insert(lines.begin() + dataEnd, row);
    }

    // Write the updated contents to a temporary file.
    string tempPath = path + ".tmp";

    ofstream output(tempPath);
    if (!output.is_open()) {
        return false;
    }

    for (const string& existingLine : lines) {
        output << existingLine << '\n';
    }

    output.close();

    if (!output) {
        fs::remove(tempPath);
        return false;
    }

    // Replace the original database file.
    error_code ec;
    fs::copy_file(
        tempPath,
        path,
        fs::copy_options::overwrite_existing,
        ec
    );

    fs::remove(tempPath);

    return !ec;
}


//SELECT from table
vector<vector<string>> StorageManager::getRows(
    const string& database,
    const string& tableName
) {
    vector<vector<string>> rows;

    string path = "data/" + database + ".db";
    ifstream file(path);

    if (!file.is_open()) {
        return rows;
    }

    string line;
    bool readingData = false;

    while (getline(file, line)) {
        if (line == "DATA " + tableName) {
            readingData = true;
            continue;
        }

        if (readingData && line == "END_DATA") {
            break;
        }

        if (readingData) {
            vector<string> row;
            stringstream ss(line);
            string value;

            while (getline(ss, value, '|')) {
                row.push_back(value);
            }

            rows.push_back(row);
        }
    }

    file.close();
    return rows;
}

