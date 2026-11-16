#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
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
        
        else {
            cout << "Error: Unknown command." << endl;
        }
    }
}
