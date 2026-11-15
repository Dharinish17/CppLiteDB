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

        transform(command.begin(), command.end(), command.begin(),
                  [](unsigned char c) { return tolower(c); });

        if (command == "exit") {
            cout << "Closing CppLiteDB..." << endl;
            break;
        }

        cout << "Command received: " << command << endl;
    }
}