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
            bool hasPrimaryKey = false;

            while (getline(ss, definition, ','))
            {
                stringstream columnStream(definition);

                Column column;

                if (!(columnStream >> column.name >> column.type))
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

                // Parse constraints
                string constraint;

                while (columnStream >> constraint)
                {
                    if (constraint == "primary")
                    {
                        string keyWord;

                        if (!(columnStream >> keyWord) || keyWord != "key")
                        {
                            valid = false;
                            break;
                        }

                        column.primaryKey = true;
                        column.notNull = true;
                    }
                    else if (constraint == "not")
                    {
                        string nullWord;

                        if (!(columnStream >> nullWord) || nullWord != "null")
                        {
                            valid = false;
                            break;
                        }

                        column.notNull = true;
                    }
                    else if (constraint == "unique")
                    {
                        column.unique = true;
                    }
                    else
                    {
                        valid = false;
                        break;
                    }
                }

                if (!valid)
                {
                    break;
                }

                // Support only one primary key column per table for now
                if (column.primaryKey)
                {
                    if (hasPrimaryKey)
                    {
                        valid = false;
                        break;
                    }

                    hasPrimaryKey = true;
                }

                columns.push_back(column);
            }

            if (!valid || columns.empty())
            {
                cout << "Error: Invalid column definition or unsupported data type/constraint."
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

        // UPDATE a row in a table
        else if (normalized.rfind("update ", 0) == 0)
        {
            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first." << endl;
                continue;
            }

            // Find SET and WHERE keywords.
            size_t setPos = normalized.find(" set ");
            size_t wherePos = normalized.find(" where ");

            if (setPos == string::npos || wherePos == string::npos ||
                wherePos <= setPos)
            {
                cout << "Error: Invalid UPDATE syntax." << endl;
                continue;
            }

            // Extract table name.
            string tableName = normalized.substr(7, setPos - 7);

            // Extract SET assignment.
            string assignment = normalized.substr(
                setPos + 5,
                wherePos - (setPos + 5));

            // Extract WHERE condition.
            string condition = normalized.substr(wherePos + 7);

            // Remove extra spaces.
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

            trim(tableName);
            trim(assignment);
            trim(condition);

            // Parse the SET assignment.
            size_t equalPos = assignment.find('=');

            if (equalPos == string::npos)
            {
                cout << "Error: Invalid SET assignment." << endl;
                continue;
            }

            string updateColumn = assignment.substr(0, equalPos);
            string updateValue = assignment.substr(equalPos + 1);

            trim(updateColumn);
            trim(updateValue);

            // Remove surrounding single quotes.
            if (updateValue.size() >= 2 &&
                updateValue.front() == '\'' &&
                updateValue.back() == '\'')
            {
                updateValue = updateValue.substr(
                    1, updateValue.size() - 2);
            }

            if (updateColumn.empty() || updateValue.empty())
            {
                cout << "Error: Invalid SET assignment." << endl;
                continue;
            }

            // Get table columns and records.
            vector<Column> columns = storage.getTableColumns(
                activeDatabase, tableName);

            if (columns.empty())
            {
                cout << "Error: Table does not exist." << endl;
                continue;
            }

            vector<vector<string>> rows = storage.getRows(
                activeDatabase, tableName);

            // Find the column to update.
            size_t updateIndex = columns.size();

            for (size_t i = 0; i < columns.size(); i++)
            {
                if (columns[i].name == updateColumn)
                {
                    updateIndex = i;
                    break;
                }
            }

            if (updateIndex == columns.size())
            {
                cout << "Error: Column '" << updateColumn
                     << "' does not exist." << endl;
                continue;
            }

            // Validate the new value for integer columns.
            string updateType = columns[updateIndex].type;

            if (updateType == "int" || updateType == "integer")
            {
                try
                {
                    size_t parsed = 0;
                    stoll(updateValue, &parsed);

                    if (parsed != updateValue.length())
                    {
                        throw invalid_argument("Invalid number");
                    }
                }
                catch (...)
                {
                    cout << "Error: Invalid integer value." << endl;
                    continue;
                }
            }

            // Identify the WHERE comparison operator.
            string op;
            size_t operatorPos = string::npos;

            if ((operatorPos = condition.find(">=")) != string::npos)
                op = ">=";
            else if ((operatorPos = condition.find("<=")) != string::npos)
                op = "<=";
            else if ((operatorPos = condition.find("!=")) != string::npos)
                op = "!=";
            else if ((operatorPos = condition.find(">")) != string::npos)
                op = ">";
            else if ((operatorPos = condition.find("<")) != string::npos)
                op = "<";
            else if ((operatorPos = condition.find("=")) != string::npos)
                op = "=";
            else
            {
                cout << "Error: Unsupported WHERE operator." << endl;
                continue;
            }

            // Extract WHERE column and expected value.
            string whereColumn = condition.substr(0, operatorPos);
            string expectedValue = condition.substr(
                operatorPos + op.length());

            trim(whereColumn);
            trim(expectedValue);

            if (whereColumn.empty() || expectedValue.empty())
            {
                cout << "Error: Invalid WHERE condition." << endl;
                continue;
            }

            // Remove surrounding single quotes.
            if (expectedValue.size() >= 2 &&
                expectedValue.front() == '\'' &&
                expectedValue.back() == '\'')
            {
                expectedValue = expectedValue.substr(
                    1, expectedValue.size() - 2);
            }

            // Find the WHERE column index.
            size_t whereIndex = columns.size();

            for (size_t i = 0; i < columns.size(); i++)
            {
                if (columns[i].name == whereColumn)
                {
                    whereIndex = i;
                    break;
                }
            }

            if (whereIndex == columns.size())
            {
                cout << "Error: Column '" << whereColumn
                     << "' does not exist." << endl;
                continue;
            }

            // Check whether the WHERE column is numeric.
            string whereType = columns[whereIndex].type;

            bool isNumeric =
                whereType == "int" || whereType == "integer";

            long long expectedNumber = 0;

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
                    cout << "Error: Invalid numeric comparison value."
                         << endl;
                    continue;
                }
            }

            // Identify matching records without modifying them.
            vector<size_t> matchingRows;

            for (size_t i = 0; i < rows.size(); i++)
            {
                const auto &row = rows[i];

                if (updateIndex >= row.size() ||
                    whereIndex >= row.size())
                {
                    continue;
                }

                bool matches = false;

                if (isNumeric)
                {
                    try
                    {
                        size_t parsed = 0;

                        long long actualNumber = stoll(
                            row[whereIndex], &parsed);

                        if (parsed != row[whereIndex].length())
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
                    string actualValue = row[whereIndex];

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
                    matchingRows.push_back(i);
                }
            }

            // Check NOT NULL constraint.
            if (columns[updateIndex].notNull &&
                !matchingRows.empty() &&
                updateValue.empty())
            {
                cout << "Error: Cannot assign an empty value to a NOT NULL column."
                     << endl;
                continue;
            }

            // Check PRIMARY KEY uniqueness.
            if (columns[updateIndex].primaryKey &&
                !matchingRows.empty())
            {
                vector<string> primaryKeyValues;

                bool duplicateKey = false;

                for (size_t i = 0; i < rows.size(); i++)
                {
                    if (updateIndex >= rows[i].size())
                    {
                        duplicateKey = true;
                        break;
                    }

                    // Use the new value for rows being updated.
                    string keyValue = rows[i][updateIndex];

                    for (size_t index : matchingRows)
                    {
                        if (index == i)
                        {
                            keyValue = updateValue;
                            break;
                        }
                    }

                    // Check for duplicate primary key values.
                    for (const string &existingKey : primaryKeyValues)
                    {
                        if (existingKey == keyValue)
                        {
                            duplicateKey = true;
                            break;
                        }
                    }

                    if (duplicateKey)
                    {
                        break;
                    }

                    primaryKeyValues.push_back(keyValue);
                }

                if (duplicateKey)
                {
                    cout << "Error: Duplicate PRIMARY KEY value."
                         << endl;
                    continue;
                }
            }

            // Apply the update after all validations pass.
            for (size_t index : matchingRows)
            {
                rows[index][updateIndex] = updateValue;
            }

            int updatedCount = static_cast<int>(matchingRows.size());

            // Save the modified records.
            if (storage.saveRows(activeDatabase, tableName, rows))
            {
                cout << updatedCount << " row(s) updated successfully."
                     << endl;
            }
            else
            {
                cout << "Error: Could not update records." << endl;
            }
        }

        // DELETE command.
        else if (normalized.rfind("delete from ", 0) == 0)
        {
            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first." << endl;
                continue;
            }

            // Find WHERE keyword.
            size_t wherePos = normalized.find(" where ");

            if (wherePos == string::npos)
            {
                cout << "Error: DELETE requires a WHERE condition." << endl;
                continue;
            }

            // Extract table name and WHERE condition.
            string tableName = normalized.substr(12, wherePos - 12);
            string condition = normalized.substr(wherePos + 7);

            // Remove extra spaces.
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

            trim(tableName);
            trim(condition);

            if (tableName.empty() || condition.empty())
            {
                cout << "Error: Invalid DELETE syntax." << endl;
                continue;
            }

            // Get table columns and records.
            vector<Column> columns = storage.getTableColumns(
                activeDatabase, tableName);

            if (columns.empty())
            {
                cout << "Error: Table does not exist." << endl;
                continue;
            }

            vector<vector<string>> rows = storage.getRows(
                activeDatabase, tableName);

            // Identify the comparison operator.
            string op;
            size_t operatorPos = string::npos;

            if ((operatorPos = condition.find(">=")) != string::npos)
                op = ">=";
            else if ((operatorPos = condition.find("<=")) != string::npos)
                op = "<=";
            else if ((operatorPos = condition.find("!=")) != string::npos)
                op = "!=";
            else if ((operatorPos = condition.find(">")) != string::npos)
                op = ">";
            else if ((operatorPos = condition.find("<")) != string::npos)
                op = "<";
            else if ((operatorPos = condition.find("=")) != string::npos)
                op = "=";
            else
            {
                cout << "Error: Unsupported WHERE operator." << endl;
                continue;
            }

            // Extract column name and expected value.
            string columnName = condition.substr(0, operatorPos);
            string expectedValue = condition.substr(
                operatorPos + op.length());

            trim(columnName);
            trim(expectedValue);

            if (columnName.empty() || expectedValue.empty())
            {
                cout << "Error: Invalid WHERE condition." << endl;
                continue;
            }

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
                    cout << "Error: Invalid numeric comparison value."
                         << endl;
                    continue;
                }
            }

            // Keep only records that do not match the condition.
            vector<vector<string>> remainingRows;
            int deletedCount = 0;

            for (const auto &row : rows)
            {
                if (columnIndex >= row.size())
                {
                    remainingRows.push_back(row);
                    continue;
                }

                bool matches = false;

                if (isNumeric)
                {
                    try
                    {
                        size_t parsed = 0;

                        long long actualNumber = stoll(
                            row[columnIndex], &parsed);

                        if (parsed != row[columnIndex].length())
                        {
                            remainingRows.push_back(row);
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
                        remainingRows.push_back(row);
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
                    deletedCount++;
                }
                else
                {
                    remainingRows.push_back(row);
                }
            }

            // Save the remaining records.
            if (storage.saveRows(
                    activeDatabase, tableName, remainingRows))
            {
                cout << deletedCount << " row(s) deleted successfully."
                     << endl;
            }
            else
            {
                cout << "Error: Could not delete records." << endl;
            }
        }

        // SELECT from table
        else if (normalized.rfind("select ", 0) == 0)
        {
            if (activeDatabase.empty())
            {
                cout << "Error: Select a database first." << endl;
                continue;
            }

            // Find FROM keyword.
            size_t fromPos = normalized.find(" from ");

            if (fromPos == string::npos)
            {
                cout << "Error: Invalid SELECT syntax." << endl;
                continue;
            }

            // Extract selected columns.
            string selectedColumnText = normalized.substr(
                7, fromPos - 7);

            // Find WHERE and ORDER BY keywords.
            size_t wherePos = normalized.find(" where ", fromPos);
            size_t orderPos = normalized.find(" order by ", fromPos);
            size_t limitPos = normalized.find(" limit ", fromPos);

            // Determine where the table name ends.
            size_t tableEnd = normalized.length();

            if (wherePos != string::npos)
            {
                tableEnd = min(tableEnd, wherePos);
            }

            if (orderPos != string::npos)
            {
                tableEnd = min(tableEnd, orderPos);
            }

            if (limitPos != string::npos)
            {
                tableEnd = min(tableEnd, limitPos);
            }

            // Extract table name.
            string tableName = normalized.substr(
                fromPos + 6,
                tableEnd - (fromPos + 6));

            // Remove extra spaces.
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

            trim(selectedColumnText);
            trim(tableName);

            // Get all table columns.
            vector<Column> columns = storage.getTableColumns(
                activeDatabase, tableName);

            if (columns.empty())
            {
                cout << "Error: Table does not exist." << endl;
                continue;
            }

            // Get all table records.
            vector<vector<string>> rows = storage.getRows(
                activeDatabase, tableName);

            // Detect aggregate functions.
            string aggregateName;
            string aggregateArgument;
            size_t aggregateColumnIndex = columns.size();

            bool isAggregate = false;

            size_t openParen = selectedColumnText.find('(');
            size_t closeParen = selectedColumnText.find(')');

            if (openParen != string::npos ||
                closeParen != string::npos)
            {
                if (openParen == string::npos ||
                    closeParen == string::npos ||
                    closeParen != selectedColumnText.length() - 1 ||
                    closeParen <= openParen)
                {
                    cout << "Error: Invalid aggregate function syntax."
                         << endl;
                    continue;
                }

                aggregateName = selectedColumnText.substr(0, openParen);
                aggregateArgument = selectedColumnText.substr(
                    openParen + 1,
                    closeParen - openParen - 1);

                trim(aggregateName);
                trim(aggregateArgument);

                if (aggregateName != "count" &&
                    aggregateName != "sum" &&
                    aggregateName != "avg" &&
                    aggregateName != "min" &&
                    aggregateName != "max")
                {
                    cout << "Error: Unsupported aggregate function."
                         << endl;
                    continue;
                }

                if (aggregateArgument.empty())
                {
                    cout << "Error: Aggregate argument is missing."
                         << endl;
                    continue;
                }

                if (aggregateName == "count" &&
                    aggregateArgument == "*")
                {
                    isAggregate = true;
                }
                else
                {
                    // Find the aggregate column.
                    for (size_t i = 0; i < columns.size(); i++)
                    {
                        if (columns[i].name == aggregateArgument)
                        {
                            aggregateColumnIndex = i;
                            break;
                        }
                    }

                    if (aggregateColumnIndex == columns.size())
                    {
                        cout << "Error: Column '" << aggregateArgument
                             << "' does not exist." << endl;
                        continue;
                    }

                    isAggregate = true;
                }

                // SUM and AVG require numeric columns.
                if (aggregateName == "sum" ||
                    aggregateName == "avg")
                {
                    string type = columns[aggregateColumnIndex].type;

                    if (type != "int" && type != "integer")
                    {
                        cout << "Error: " << aggregateName
                             << "() requires a numeric column."
                             << endl;
                        continue;
                    }
                }
            }

            // Determine which columns should be displayed.
            vector<size_t> selectedIndexes;

            if (isAggregate)
            {
                // Aggregate queries produce one calculated result.
            }
            else if (selectedColumnText == "*")
            {
                for (size_t i = 0; i < columns.size(); i++)
                {
                    selectedIndexes.push_back(i);
                }
            }
            else
            {
                stringstream columnStream(selectedColumnText);
                string columnName;

                while (getline(columnStream, columnName, ','))
                {
                    trim(columnName);

                    if (columnName.empty())
                    {
                        cout << "Error: Invalid column list." << endl;
                        selectedIndexes.clear();
                        break;
                    }

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

                        selectedIndexes.clear();
                        break;
                    }

                    selectedIndexes.push_back(columnIndex);
                }

                if (selectedIndexes.empty())
                {
                    continue;
                }
            }

            // Apply WHERE condition if provided.
            if (wherePos != string::npos)
            {
                size_t conditionEnd = normalized.length();

                if (orderPos != string::npos && orderPos > wherePos)
                {
                    conditionEnd = min(conditionEnd, orderPos);
                }

                if (limitPos != string::npos && limitPos > wherePos)
                {
                    conditionEnd = min(conditionEnd, limitPos);
                }

                string condition = normalized.substr(
                    wherePos + 7,
                    conditionEnd - (wherePos + 7));

                // Split conditions using AND or OR.
                auto splitConditions = [](const string &text,
                                          const string &delimiter)
                {
                    vector<string> parts;
                    size_t start = 0;
                    size_t pos;

                    while ((pos = text.find(delimiter, start)) !=
                           string::npos)
                    {
                        parts.push_back(text.substr(start, pos - start));
                        start = pos + delimiter.length();
                    }

                    parts.push_back(text.substr(start));

                    return parts;
                };

                // Store parsed condition information.
                struct ParsedCondition
                {
                    size_t columnIndex;
                    string op;
                    string expectedValue;
                    bool isNumeric;
                    long long expectedNumber;
                };

                vector<vector<ParsedCondition>> conditionGroups;

                string errorMessage;

                // Split the complete condition using OR.
                vector<string> orGroups = splitConditions(
                    condition, " or ");

                for (const string &orGroup : orGroups)
                {
                    vector<ParsedCondition> andConditions;

                    // Split each OR group using AND.
                    vector<string> parts = splitConditions(
                        orGroup, " and ");

                    for (string part : parts)
                    {
                        trim(part);

                        // Identify comparison operator.
                        string op;
                        size_t operatorPos = string::npos;

                        if ((operatorPos = part.find(">=")) != string::npos)
                            op = ">=";
                        else if ((operatorPos = part.find("<=")) != string::npos)
                            op = "<=";
                        else if ((operatorPos = part.find("!=")) != string::npos)
                            op = "!=";
                        else if ((operatorPos = part.find(">")) != string::npos)
                            op = ">";
                        else if ((operatorPos = part.find("<")) != string::npos)
                            op = "<";
                        else if ((operatorPos = part.find("=")) != string::npos)
                            op = "=";
                        else
                        {
                            errorMessage = "Unsupported comparison operator.";
                            break;
                        }

                        // Extract column name and expected value.
                        string columnName = part.substr(0, operatorPos);
                        string expectedValue = part.substr(
                            operatorPos + op.length());

                        trim(columnName);
                        trim(expectedValue);

                        if (columnName.empty() || expectedValue.empty())
                        {
                            errorMessage = "Invalid WHERE condition.";
                            break;
                        }

                        // Remove surrounding single quotes.
                        if (expectedValue.size() >= 2 &&
                            expectedValue.front() == '\'' &&
                            expectedValue.back() == '\'')
                        {
                            expectedValue = expectedValue.substr(
                                1, expectedValue.size() - 2);
                        }

                        // Find the WHERE column index.
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
                            errorMessage = "Column '" + columnName +
                                           "' does not exist.";
                            break;
                        }

                        // Check whether the column is numeric.
                        string columnType = columns[columnIndex].type;

                        bool isNumeric =
                            columnType == "int" ||
                            columnType == "integer";

                        long long expectedNumber = 0;

                        if (isNumeric)
                        {
                            try
                            {
                                size_t parsed = 0;

                                expectedNumber = stoll(
                                    expectedValue, &parsed);

                                if (parsed != expectedValue.length())
                                {
                                    throw invalid_argument("Invalid number");
                                }
                            }
                            catch (...)
                            {
                                errorMessage =
                                    "Invalid numeric comparison value.";
                                break;
                            }
                        }

                        andConditions.push_back({columnIndex,
                                                 op,
                                                 expectedValue,
                                                 isNumeric,
                                                 expectedNumber});
                    }

                    if (!errorMessage.empty())
                    {
                        break;
                    }

                    conditionGroups.push_back(andConditions);
                }

                if (!errorMessage.empty())
                {
                    cout << "Error: " << errorMessage << endl;
                    continue;
                }

                // Filter matching records.
                vector<vector<string>> filteredRows;

                for (const auto &row : rows)
                {
                    bool rowMatchesAnyGroup = false;

                    for (const auto &group : conditionGroups)
                    {
                        bool groupMatches = true;

                        for (const auto &current : group)
                        {
                            if (current.columnIndex >= row.size())
                            {
                                groupMatches = false;
                                break;
                            }

                            bool matches = false;

                            if (current.isNumeric)
                            {
                                try
                                {
                                    size_t parsed = 0;

                                    long long actualNumber = stoll(
                                        row[current.columnIndex], &parsed);

                                    if (parsed != row[current.columnIndex].length())
                                    {
                                        groupMatches = false;
                                        break;
                                    }

                                    long long expectedNumber =
                                        current.expectedNumber;

                                    if (current.op == "=")
                                        matches = actualNumber == expectedNumber;
                                    else if (current.op == "!=")
                                        matches = actualNumber != expectedNumber;
                                    else if (current.op == ">")
                                        matches = actualNumber > expectedNumber;
                                    else if (current.op == "<")
                                        matches = actualNumber < expectedNumber;
                                    else if (current.op == ">=")
                                        matches = actualNumber >= expectedNumber;
                                    else if (current.op == "<=")
                                        matches = actualNumber <= expectedNumber;
                                }
                                catch (...)
                                {
                                    groupMatches = false;
                                    break;
                                }
                            }
                            else
                            {
                                string actualValue = row[current.columnIndex];
                                string expectedValue = current.expectedValue;

                                if (current.op == "=")
                                    matches = actualValue == expectedValue;
                                else if (current.op == "!=")
                                    matches = actualValue != expectedValue;
                                else if (current.op == ">")
                                    matches = actualValue > expectedValue;
                                else if (current.op == "<")
                                    matches = actualValue < expectedValue;
                                else if (current.op == ">=")
                                    matches = actualValue >= expectedValue;
                                else if (current.op == "<=")
                                    matches = actualValue <= expectedValue;
                            }

                            if (!matches)
                            {
                                groupMatches = false;
                                break;
                            }
                        }

                        if (groupMatches)
                        {
                            rowMatchesAnyGroup = true;
                            break;
                        }
                    }

                    if (rowMatchesAnyGroup)
                    {
                        filteredRows.push_back(row);
                    }
                }

                rows = filteredRows;
            }

            // Apply ORDER BY if provided.
            if (orderPos != string::npos)
            {
                size_t orderEnd = normalized.length();

                if (limitPos != string::npos && limitPos > orderPos)
                {
                    orderEnd = limitPos;
                }

                string orderClause = normalized.substr(
                    orderPos + 10,
                    orderEnd - (orderPos + 10));

                trim(orderClause);

                // Extract column name and sorting direction.
                stringstream orderStream(orderClause);

                string orderColumn;
                string direction;
                string extra;

                orderStream >> orderColumn;
                orderStream >> direction;
                orderStream >> extra;

                if (orderColumn.empty() || !extra.empty() ||
                    (!direction.empty() &&
                     direction != "asc" && direction != "desc"))
                {
                    cout << "Error: Invalid ORDER BY syntax." << endl;
                    continue;
                }

                // Find the sorting column.
                size_t orderIndex = columns.size();

                for (size_t i = 0; i < columns.size(); i++)
                {
                    if (columns[i].name == orderColumn)
                    {
                        orderIndex = i;
                        break;
                    }
                }

                if (orderIndex == columns.size())
                {
                    cout << "Error: Column '" << orderColumn
                         << "' does not exist." << endl;
                    continue;
                }

                // Determine sorting direction.
                bool descending = direction == "desc";

                // Determine whether the column is numeric.
                string orderType = columns[orderIndex].type;

                bool isNumeric =
                    orderType == "int" || orderType == "integer";

                // Sort the filtered records.
                try
                {
                    // Validate numeric values before sorting.
                    if (isNumeric)
                    {
                        for (const auto &row : rows)
                        {
                            if (orderIndex >= row.size())
                            {
                                throw invalid_argument("Invalid row");
                            }

                            size_t parsed = 0;

                            stoll(row[orderIndex], &parsed);

                            if (parsed != row[orderIndex].length())
                            {
                                throw invalid_argument("Invalid number");
                            }
                        }
                    }

                    stable_sort(
                        rows.begin(),
                        rows.end(),
                        [&](const vector<string> &a,
                            const vector<string> &b)
                        {
                            if (isNumeric)
                            {
                                long long valueA = stoll(a[orderIndex]);
                                long long valueB = stoll(b[orderIndex]);

                                return descending
                                           ? valueA > valueB
                                           : valueA < valueB;
                            }

                            return descending
                                       ? a[orderIndex] > b[orderIndex]
                                       : a[orderIndex] < b[orderIndex];
                        });
                }
                catch (...)
                {
                    cout << "Error: Unable to sort records." << endl;
                    continue;
                }
            }

            // Apply LIMIT if provided.
            if (limitPos != string::npos)
            {
                string limitText = normalized.substr(limitPos + 7);

                trim(limitText);

                // Convert LIMIT value to an integer.
                size_t parsed = 0;
                long long limitValue = 0;

                try
                {
                    limitValue = stoll(limitText, &parsed);

                    if (parsed != limitText.length() ||
                        limitValue < 0)
                    {
                        throw invalid_argument("Invalid LIMIT");
                    }
                }
                catch (...)
                {
                    cout << "Error: LIMIT must be a non-negative integer."
                         << endl;
                    continue;
                }

                // Keep only the requested number of records.
                if (static_cast<unsigned long long>(limitValue) < rows.size())
                {
                    rows.resize(static_cast<size_t>(limitValue));
                }
            }

            // Calculate aggregate result.
            if (isAggregate)
            {
                string result;

                if (aggregateName == "count")
                {
                    result = to_string(rows.size());

                    if (aggregateArgument != "*")
                    {
                        size_t count = 0;

                        for (const auto &row : rows)
                        {
                            if (aggregateColumnIndex < row.size() &&
                                !row[aggregateColumnIndex].empty())
                            {
                                count++;
                            }
                        }

                        result = to_string(count);
                    }
                }
                else
                {
                    if (rows.empty())
                    {
                        cout << "Error: Cannot calculate "
                             << aggregateName
                             << "() on an empty result." << endl;
                        continue;
                    }

                    bool isNumeric =
                        columns[aggregateColumnIndex].type == "int" ||
                        columns[aggregateColumnIndex].type == "integer";

                    if (aggregateName == "sum" ||
                        aggregateName == "avg" ||
                        (isNumeric &&
                         (aggregateName == "min" ||
                          aggregateName == "max")))
                    {
                        long long total = 0;
                        long long minimum = 0;
                        long long maximum = 0;

                        bool first = true;

                        try
                        {
                            for (const auto &row : rows)
                            {
                                if (aggregateColumnIndex >= row.size())
                                {
                                    throw invalid_argument("Invalid row");
                                }

                                size_t parsed = 0;

                                long long value = stoll(
                                    row[aggregateColumnIndex], &parsed);

                                if (parsed !=
                                    row[aggregateColumnIndex].length())
                                {
                                    throw invalid_argument("Invalid number");
                                }

                                total += value;

                                if (first)
                                {
                                    minimum = value;
                                    maximum = value;
                                    first = false;
                                }
                                else
                                {
                                    minimum = min(minimum, value);
                                    maximum = max(maximum, value);
                                }
                            }
                        }
                        catch (...)
                        {
                            cout << "Error: Invalid numeric data."
                                 << endl;
                            continue;
                        }

                        if (aggregateName == "sum")
                        {
                            result = to_string(total);
                        }
                        else if (aggregateName == "avg")
                        {
                            double average =
                                static_cast<double>(total) / rows.size();

                            stringstream output;
                            output << fixed << setprecision(2) << average;

                            result = output.str();
                        }
                        else if (aggregateName == "min")
                        {
                            result = to_string(minimum);
                        }
                        else if (aggregateName == "max")
                        {
                            result = to_string(maximum);
                        }
                    }
                    else
                    {
                        // MIN and MAX for string columns.
                        string minimum;
                        string maximum;

                        bool first = true;

                        for (const auto &row : rows)
                        {
                            if (aggregateColumnIndex >= row.size())
                            {
                                cout << "Error: Invalid row data."
                                     << endl;
                                first = false;
                                break;
                            }

                            string value = row[aggregateColumnIndex];

                            if (first)
                            {
                                minimum = value;
                                maximum = value;
                                first = false;
                            }
                            else
                            {
                                minimum = min(minimum, value);
                                maximum = max(maximum, value);
                            }
                        }

                        if (first)
                        {
                            cout << "Error: Invalid row data." << endl;
                            continue;
                        }

                        result = aggregateName == "min"
                                     ? minimum
                                     : maximum;
                    }
                }

                // Display aggregate result.
                cout << left << setw(20)
                     << selectedColumnText << endl;

                cout << left << setw(20) << result << endl;

                cout << "1 row selected." << endl;

                continue;
            }

            // Calculate column widths.
            vector<int> widths(selectedIndexes.size(), 0);

            for (size_t i = 0; i < selectedIndexes.size(); i++)
            {
                widths[i] = static_cast<int>(
                    columns[selectedIndexes[i]].name.length());
            }

            for (const auto &row : rows)
            {
                for (size_t i = 0; i < selectedIndexes.size(); i++)
                {
                    size_t index = selectedIndexes[i];

                    if (index < row.size())
                    {
                        widths[i] = max(
                            widths[i],
                            static_cast<int>(row[index].length()));
                    }
                }
            }

            // Display column headers.
            for (size_t i = 0; i < selectedIndexes.size(); i++)
            {
                cout << left << setw(widths[i] + 2)
                     << columns[selectedIndexes[i]].name;
            }

            cout << endl;

            // Display selected records.
            for (const auto &row : rows)
            {
                for (size_t i = 0; i < selectedIndexes.size(); i++)
                {
                    size_t index = selectedIndexes[i];

                    string value = index < row.size() ? row[index] : "";

                    cout << left << setw(widths[i] + 2) << value;
                }

                cout << endl;
            }

            cout << rows.size() << " row(s) selected." << endl;
        }

        else
        {
            cout << "Error: Unknown command." << endl;
        }
    }
}
