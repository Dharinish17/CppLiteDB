#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "DatabaseManager.h"

using namespace std;

void DatabaseManager::start()
{
    cout << "Welcome to CppLiteDB!" << endl;
    cout << "SQLite-inspired Database Engine" << endl;
    cout << "Type EXIT to close the database." << endl;

    string command;

    while (true)
    {
        // Display prompt
        if (activeDatabase.empty())
        {
            cout << "CppLiteDB> ";
        }
        else
        {
            cout << "CppLiteDB [" << activeDatabase << "]> ";
        }

        getline(cin, command);

        string normalized = command;

        transform(normalized.begin(), normalized.end(), normalized.begin(),
                  [](unsigned char c)
                  {
                      return tolower(c);
                  });

        if (!normalized.empty() && normalized.back() == ';')
        {
            normalized.pop_back();
        }

        if (normalized == "exit")
        {
            cout << "Closing CppLiteDB..." << endl;
            break;
        }

        // CREATE
        else if (normalized.find("create database ") == 0)
        {

            string name = normalized.substr(16);

            if (storage.createDatabase(name))
            {
                cout << "Database '" << name
                     << "' created successfully." << endl;
            }
            else
            {
                cout << "Error: Database already exists or could not be created."
                     << endl;
            }
        }

        // SHOW
        else if (normalized == "show databases")
        {

            vector<string> databases = storage.getDatabases();

            if (databases.empty())
            {
                cout << "No databases found." << endl;
            }
            else
            {
                cout << "Databases:" << endl;

                for (size_t i = 0; i < databases.size(); i++)
                {
                    cout << i + 1 << ". " << databases[i] << endl;
                }
            }
        }

        // USE
        else if (normalized.find("use ") == 0)
        {

            string name = normalized.substr(4);

            if (storage.databaseExists(name))
            {

                activeDatabase = name;

                cout << "Database changed to '"
                     << activeDatabase << "'." << endl;
            }
            else
            {
                cout << "Error: Database '" << name
                     << "' does not exist." << endl;
            }
        }

        // CREATE table
        else if (normalized.find("create table ") == 0)
        {

            // A database must be selected first
            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            // Extract the table definition
            size_t openBracket = normalized.find('(');
            size_t closeBracket = normalized.rfind(')');

            if (openBracket == string::npos ||
                closeBracket == string::npos ||
                closeBracket < openBracket)
            {

                cout << "Error: Invalid CREATE TABLE syntax." << endl;
                continue;
            }

            // Extract table name
            string tableName = normalized.substr(
                13, openBracket - 13);

            // Remove extra spaces from table name
            size_t first = tableName.find_first_not_of(" \t");
            size_t last = tableName.find_last_not_of(" \t");

            if (first == string::npos || last == string::npos)
            {
                cout << "Error: Invalid table name." << endl;
                continue;
            }

            tableName = tableName.substr(first, last - first + 1);

            // Extract column definitions
            string columnDefinitions = normalized.substr(
                openBracket + 1,
                closeBracket - openBracket - 1);

            vector<Column> columns;
            stringstream ss(columnDefinitions);
            string definition;

            bool valid = true;

            while (getline(ss, definition, ','))
            {

                stringstream columnStream(definition);

                Column column;

                if (!(columnStream >> column.name >> column.type))
                {
                    valid = false;
                    break;
                }

                string extra;

                if (columnStream >> extra)
                {
                    valid = false;
                    break;
                }

                // Check supported data types
                if (column.type != "int" &&
                    column.type != "integer" &&
                    column.type.find("varchar(") != 0)
                {

                    valid = false;
                    break;
                }

                columns.push_back(column);
            }

            if (!valid || columns.empty())
            {
                cout << "Error: Invalid column definition or unsupported data type."
                     << endl;
                continue;
            }

            if (storage.createTable(activeDatabase, tableName, columns))
            {
                cout << "Table '" << tableName
                     << "' created successfully." << endl;
            }
            else
            {
                cout << "Error: Table already exists or could not be created."
                     << endl;
            }
        }

        // SHOW TABLE
        else if (normalized == "show tables")
        {

            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            vector<string> tables = storage.getTables(activeDatabase);

            if (tables.empty())
            {
                cout << "No tables found." << endl;
            }
            else
            {
                cout << "Tables in database '" << activeDatabase << "':"
                     << endl;

                for (size_t i = 0; i < tables.size(); i++)
                {
                    cout << i + 1 << ". " << tables[i] << endl;
                }
            }
        }

        // DESCRIBE table
        else if (normalized.find("describe ") == 0)
        {

            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            string tableName = normalized.substr(9);

            if (tableName.empty())
            {
                cout << "Error: Please provide a table name." << endl;
                continue;
            }

            vector<Column> columns =
                storage.getTableColumns(activeDatabase, tableName);

            if (columns.empty())
            {
                cout << "Error: Table '" << tableName
                     << "' does not exist or could not be read." << endl;
                continue;
            }

            cout << "\nTable: " << tableName << endl;

            cout << left << setw(20) << "Field"
                 << setw(20) << "Type" << endl;

            cout << string(40, '-') << endl;

            for (const auto &column : columns)
            {
                cout << left << setw(20) << column.name
                     << setw(20) << column.type << endl;
            }
        }

        // INSERT into table
        else if (normalized.find("insert into ") == 0)
        {
            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            // Find the VALUES keyword.
            size_t valuesPos = normalized.find(" values ");

            if (valuesPos == string::npos)
            {
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
                closeBracket <= openBracket)
            {
                cout << "Error: Invalid VALUES syntax." << endl;
                continue;
            }

            string valuesPart = normalized.substr(
                openBracket + 1,
                closeBracket - openBracket - 1);

            // Split values using commas.
            vector<string> values;
            stringstream ss(valuesPart);
            string value;

            while (getline(ss, value, ','))
            {
                size_t first = value.find_first_not_of(" \t");
                size_t last = value.find_last_not_of(" \t");

                if (first == string::npos)
                {
                    values.clear();
                    break;
                }

                value = value.substr(first, last - first + 1);

                // Remove surrounding single quotes.
                if (value.size() >= 2 &&
                    value.front() == '\'' &&
                    value.back() == '\'')
                {
                    value = value.substr(1, value.size() - 2);
                }

                values.push_back(value);
            }

            if (values.empty())
            {
                cout << "Error: No valid values provided." << endl;
                continue;
            }

            if (storage.insertRow(activeDatabase, tableName, values))
            {
                cout << "1 row inserted successfully." << endl;
            }
            else
            {
                cout << "Error: Could not insert row. Check the table and database." << endl;
            }
        }

        // SELECT from table

        else if (normalized.find("select * from ") == 0)
        {
            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first using USE." << endl;
                continue;
            }

            // Find the WHERE keyword.
            size_t wherePos = normalized.find(" where ");

            // Extract table name.
            string tableName;

            if (wherePos == string::npos)
            {
                tableName = normalized.substr(14);
            }
            else
            {
                tableName = normalized.substr(14, wherePos - 14);
            }

            // Remove extra spaces from table name.
            size_t first = tableName.find_first_not_of(" \t");
            size_t last = tableName.find_last_not_of(" \t");

            if (first == string::npos)
            {
                cout << "Error: Please provide a table name." << endl;
                continue;
            }

            tableName = tableName.substr(first, last - first + 1);

            // Retrieve table structure.
            vector<Column> columns =
                storage.getTableColumns(activeDatabase, tableName);

            if (columns.empty())
            {
                cout << "Error: Table '" << tableName << "' does not exist." << endl;
                continue;
            }

            // Retrieve all records.
            vector<vector<string>> rows =
                storage.getRows(activeDatabase, tableName);

            // Apply WHERE condition if provided.
            if (wherePos != string::npos)
            {
                string condition = normalized.substr(wherePos + 7);

                // Identify the comparison operator.
                string op;
                size_t operatorPos = string::npos;

                if ((operatorPos = condition.find(">=")) != string::npos)
                {
                    op = ">=";
                }
                else if ((operatorPos = condition.find("<=")) != string::npos)
                {
                    op = "<=";
                }
                else if ((operatorPos = condition.find("!=")) != string::npos)
                {
                    op = "!=";
                }
                else if ((operatorPos = condition.find(">")) != string::npos)
                {
                    op = ">";
                }
                else if ((operatorPos = condition.find("<")) != string::npos)
                {
                    op = "<";
                }
                else if ((operatorPos = condition.find("=")) != string::npos)
                {
                    op = "=";
                }
                else
                {
                    cout << "Error: Unsupported comparison operator." << endl;
                    continue;
                }

                // Extract column name and expected value.
                string columnName = condition.substr(0, operatorPos);
                string expectedValue = condition.substr(
                    operatorPos + op.length());

                // Trim spaces.
                auto trim = [](string &value)
                {
                    size_t start = value.find_first_not_of(" \t");
                    size_t end = value.find_last_not_of(" \t");

                    if (start == string::npos)
                    {
                        value.clear();
                    }
                    else
                    {
                        value = value.substr(start, end - start + 1);
                    }
                };

                trim(columnName);
                trim(expectedValue);

                // Remove surrounding single quotes.
                if (expectedValue.size() >= 2 &&
                    expectedValue.front() == '\'' &&
                    expectedValue.back() == '\'')
                {
                    expectedValue = expectedValue.substr(
                        1, expectedValue.size() - 2);
                }

                // Find the column index.
                size_t columnIndex = columns.size();

                for (size_t i = 0; i < columns.size(); i++)
                {
                    if (columns[i].name == columnName)
                    {
                        columnIndex = i;
                        break;
                    }
                }

                if (columnIndex == columns.size())
                {
                    cout << "Error: Column '" << columnName
                         << "' does not exist." << endl;
                    continue;
                }

                // Check whether the column is numeric.
                string columnType = columns[columnIndex].type;

                bool isNumeric =
                    columnType == "int" || columnType == "integer";

                long long expectedNumber = 0;

                // Convert expected value to a number if necessary.
                if (isNumeric)
                {
                    try
                    {
                        size_t parsed = 0;
                        expectedNumber = stoll(expectedValue, &parsed);

                        if (parsed != expectedValue.length())
                        {
                            throw invalid_argument("Invalid number");
                        }
                    }
                    catch (...)
                    {
                        cout << "Error: Invalid numeric comparison value." << endl;
                        continue;
                    }
                }

                // Filter matching records.
                vector<vector<string>> filteredRows;

                for (const auto &row : rows)
                {
                    if (columnIndex >= row.size())
                    {
                        continue;
                    }

                    bool matches = false;

                    if (isNumeric)
                    {
                        try
                        {
                            size_t parsed = 0;

                            long long actualNumber =
                                stoll(row[columnIndex], &parsed);

                            if (parsed != row[columnIndex].length())
                            {
                                continue;
                            }

                            if (op == "=")
                                matches = actualNumber == expectedNumber;
                            else if (op == "!=")
                                matches = actualNumber != expectedNumber;
                            else if (op == ">")
                                matches = actualNumber > expectedNumber;
                            else if (op == "<")
                                matches = actualNumber < expectedNumber;
                            else if (op == ">=")
                                matches = actualNumber >= expectedNumber;
                            else if (op == "<=")
                                matches = actualNumber <= expectedNumber;
                        }
                        catch (...)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        string actualValue = row[columnIndex];

                        if (op == "=")
                            matches = actualValue == expectedValue;
                        else if (op == "!=")
                            matches = actualValue != expectedValue;
                        else if (op == ">")
                            matches = actualValue > expectedValue;
                        else if (op == "<")
                            matches = actualValue < expectedValue;
                        else if (op == ">=")
                            matches = actualValue >= expectedValue;
                        else if (op == "<=")
                            matches = actualValue <= expectedValue;
                    }

                    if (matches)
                    {
                        filteredRows.push_back(row);
                    }
                }

                rows = filteredRows;
            }

            // Calculate column widths.
            vector<size_t> widths;

            for (const auto &column : columns)
            {
                widths.push_back(column.name.length());
            }

            for (const auto &row : rows)
            {
                for (size_t i = 0; i < row.size() && i < widths.size(); i++)
                {
                    widths[i] = max(widths[i], row[i].length());
                }
            }

            // Print table header.
            cout << endl;

            for (size_t width : widths)
            {
                cout << "+-" << string(width, '-') << "-";
            }
            cout << "+" << endl;

            cout << "|";

            for (size_t i = 0; i < columns.size(); i++)
            {
                cout << " " << left << setw(widths[i])
                     << columns[i].name << " |";
            }

            cout << endl;

            for (size_t width : widths)
            {
                cout << "+-" << string(width, '-') << "-";
            }
            cout << "+" << endl;

            // Print records.
            for (const auto &row : rows)
            {
                cout << "|";

                for (size_t i = 0; i < columns.size(); i++)
                {
                    string value = (i < row.size()) ? row[i] : "";

                    cout << " " << left << setw(widths[i])
                         << value << " |";
                }

                cout << endl;
            }

            for (size_t width : widths)
            {
                cout << "+-" << string(width, '-') << "-";
            }
            cout << "+" << endl;

            cout << rows.size() << " rows returned." << endl;
        }

        else
        {
            cout << "Error: Unknown command." << endl;
        }
    }
}
