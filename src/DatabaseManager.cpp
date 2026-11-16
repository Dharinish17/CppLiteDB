#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>
#include <iomanip>
#include "DatabaseManager.h"

using namespace std;

void DatabaseManager::start() {
    cout << "Welcome to CppLiteDB!" << endl;
    cout << "SQLite-inspired Database Engine" << endl;
    cout << "Type EXIT to close the database." << endl;

    string command;

    while (true) {
        // Display prompt
        if (activeDatabase.empty()) {
            cout << "CppLiteDB> ";
        } else {
            cout << "CppLiteDB [" << activeDatabase << "]> ";
        }

        getline(cin, command);

        string normalized = command;

        transform(normalized.begin(), normalized.end(), normalized.begin(),
                  [](unsigned char c) {
                      return tolower(c);
                  });

        if (!normalized.empty() && normalized.back() == ';') {
            normalized.pop_back();
        }

        if (normalized == "exit") {
            cout << "Closing CppLiteDB..." << endl;
            break;
        }

        //CREATE
        else if (normalized.find("create database ") == 0) {

            string name = normalized.substr(16);

            if (storage.createDatabase(name)) {
                cout << "Database '" << name
                     << "' created successfully." << endl;
            } else {
                cout << "Error: Database already exists or could not be created."
                     << endl;
            }
        }

        //SHOW
        else if (normalized == "show databases") {

            vector<string> databases = storage.getDatabases();

            if (databases.empty()) {
                cout << "No databases found." << endl;
            } else {
                cout << "Databases:" << endl;

                for (size_t i = 0; i < databases.size(); i++) {
                    cout << i + 1 << ". " << databases[i] << endl;
                }
            }
        }

        //USE
        else if (normalized.find("use ") == 0) {

            string name = normalized.substr(4);

            if (storage.databaseExists(name)) {

                activeDatabase = name;

                cout << "Database changed to '"
                     << activeDatabase << "'." << endl;

            } else {
                cout << "Error: Database '" << name
                     << "' does not exist." << endl;
            }
        }

        //CREATE table
        else if (normalized.find("create table ") == 0) {

            // A database must be selected first
            if (activeDatabase.empty()) {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            // Extract the table definition
            size_t openBracket = normalized.find('(');
            size_t closeBracket = normalized.rfind(')');

            if (openBracket == string::npos ||
                closeBracket == string::npos ||
                closeBracket < openBracket) {

                cout << "Error: Invalid CREATE TABLE syntax." << endl;
                continue;
            }

            // Extract table name
            string tableName = normalized.substr(
                13, openBracket - 13
            );

            // Remove extra spaces from table name
            size_t first = tableName.find_first_not_of(" \t");
            size_t last = tableName.find_last_not_of(" \t");

            if (first == string::npos || last == string::npos) {
                cout << "Error: Invalid table name." << endl;
                continue;
            }

            tableName = tableName.substr(first, last - first + 1);

            // Extract column definitions
            string columnDefinitions = normalized.substr(
                openBracket + 1,
                closeBracket - openBracket - 1
            );

            vector<Column> columns;
            stringstream ss(columnDefinitions);
            string definition;

            bool valid = true;

            while (getline(ss, definition, ',')) {

                stringstream columnStream(definition);

                Column column;

                if (!(columnStream >> column.name >> column.type)) {
                    valid = false;
                    break;
                }

                string extra;

                if (columnStream >> extra) {
                    valid = false;
                    break;
                }

                // Check supported data types
                if (column.type != "int" &&
                    column.type != "integer" &&
                    column.type.find("varchar(") != 0) {

                    valid = false;
                    break;
                }

                columns.push_back(column);
            }

            if (!valid || columns.empty()) {
                cout << "Error: Invalid column definition or unsupported data type."
                    << endl;
                continue;
            }

            if (storage.createTable(activeDatabase, tableName, columns)) {
                cout << "Table '" << tableName
                    << "' created successfully." << endl;
            } else {
                cout << "Error: Table already exists or could not be created."
                    << endl;
            }
        }

        //SHOW TABLE
        else if (normalized == "show tables") {

            if (activeDatabase.empty()) {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            vector<string> tables = storage.getTables(activeDatabase);

            if (tables.empty()) {
                cout << "No tables found." << endl;
            } else {
                cout << "Tables in database '" << activeDatabase << "':"
                    << endl;

                for (size_t i = 0; i < tables.size(); i++) {
                    cout << i + 1 << ". " << tables[i] << endl;
                }
            }
        }

        //DESCRIBE table
        else if (normalized.find("describe ") == 0) {

            if (activeDatabase.empty()) {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            string tableName = normalized.substr(9);

            if (tableName.empty()) {
                cout << "Error: Please provide a table name." << endl;
                continue;
            }

            vector<Column> columns =
                storage.getTableColumns(activeDatabase, tableName);

            if (columns.empty()) {
                cout << "Error: Table '" << tableName
                    << "' does not exist or could not be read." << endl;
                continue;
            }

            cout << "\nTable: " << tableName << endl;

            cout << left << setw(20) << "Field"
                << setw(20) << "Type" << endl;

            cout << string(40, '-') << endl;

            for (const auto& column : columns) {
                cout << left << setw(20) << column.name
                    << setw(20) << column.type << endl;
            }
        }

        //INSERT into table
        else if (normalized.find("insert into ") == 0) {
            if (activeDatabase.empty()) {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            // Find the VALUES keyword.
            size_t valuesPos = normalized.find(" values ");

            if (valuesPos == string::npos) {
                cout << "Error: Invalid INSERT syntax." << endl;
                continue;
            }

            // Extract table name.
            string tableName = normalized.substr(12, valuesPos - 12);

            // Extract values inside parentheses.
            size_t openBracket = normalized.find('(', valuesPos);
            size_t closeBracket = normalized.rfind(')');

            if (openBracket == string::npos ||
                closeBracket == string::npos ||
                closeBracket <= openBracket) {
                cout << "Error: Invalid VALUES syntax." << endl;
                continue;
            }

            string valuesPart = normalized.substr(
                openBracket + 1,
                closeBracket - openBracket - 1
            );

            // Split values using commas.
            vector<string> values;
            stringstream ss(valuesPart);
            string value;

            while (getline(ss, value, ',')) {
                size_t first = value.find_first_not_of(" \t");
                size_t last = value.find_last_not_of(" \t");

                if (first == string::npos) {
                    values.clear();
                    break;
                }

                value = value.substr(first, last - first + 1);

                // Remove surrounding single quotes.
                if (value.size() >= 2 &&
                    value.front() == '\'' &&
                    value.back() == '\'') {
                    value = value.substr(1, value.size() - 2);
                }

                values.push_back(value);
            }

            if (values.empty()) {
                cout << "Error: No valid values provided." << endl;
                continue;
            }

            if (storage.insertRow(activeDatabase, tableName, values)) {
                cout << "1 row inserted successfully." << endl;
            }
            else {
                cout << "Error: Could not insert row. Check the table and database." << endl;
            }
        }

        else {
            cout << "Error: Unknown command." << endl;
        }
    }
}
