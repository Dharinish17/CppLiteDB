#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "DatabaseManager.h"

using namespace std;

void DatabaseManager::start() {
    cout << "Welcome to CppLiteDB!" << endl;
    cout << "SQLite-inspired Database Engine" << endl;
    cout << "Type EXIT to close the database." << endl;

    string command;

    while (true) {
        cout << "CppLiteDB> ";
        getline(cin, command);

        string normalized = command;

        transform(normalized.begin(), normalized.end(), normalized.begin(),
                  [](unsigned char c) { return tolower(c); });

        if (normalized == "exit" || normalized == "exit;") {
            cout << "Closing CppLiteDB..." << endl;
            break;
        }

        if (normalized.find("create database ") == 0) {

            string name = normalized.substr(16);

            if (!name.empty() && name.back() == ';') {
                name.pop_back();
            }

            if (storage.createDatabase(name)) {
                cout << "Database '" << name << "' created successfully." << endl;
            } else {
                cout << "Error: Database already exists or could not be created." << endl;
            }

        } else {
            cout << "Error: Unknown command." << endl;
        }
    }
}
