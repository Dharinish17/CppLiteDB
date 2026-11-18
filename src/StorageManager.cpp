#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include "StorageManager.h"

using namespace std;
namespace fs = filesystem;

bool StorageManager::createDatabase(const string &name)
{

    fs::create_directories("data");

    string path = "data/" + name + ".db";

    if (fs::exists(path))
    {
        return false;
    }

    ofstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    file << "CPPLITEDB\n";
    file << "VERSION 1\n";

    file.close();

    return true;
}

bool StorageManager::databaseExists(const string &name)
{

    string path = "data/" + name + ".db";

    return fs::exists(path);
}

vector<string> StorageManager::getDatabases() const
{

    vector<string> databases;

    if (!fs::exists("data"))
    {
        return databases;
    }

    for (const auto &entry : fs::directory_iterator("data"))
    {

        if (entry.is_regular_file() &&
            entry.path().extension() == ".db")
        {

            databases.push_back(entry.path().stem().string());
        }
    }

    return databases;
}

bool StorageManager::createTable(const string &database, const string &tableName, const vector<Column> &columns)
{
    string path = "data/" + database + ".db";

    if (!fs::exists(path))
    {
        return false;
    }

    // Check whether the table already exists
    ifstream input(path);
    string line;

    while (getline(input, line))
    {
        if (line == "TABLE " + tableName)
        {
            input.close();
            return false;
        }
    }

    input.close();

    // Append the table schema to the database file
    ofstream output(path, ios::app);

    if (!output.is_open())
    {
        return false;
    }

    output << "TABLE " << tableName << "\n";

    for (const auto &column : columns)
    {
        output << "COLUMN "
               << column.name << " "
               << column.type;

        if (column.primaryKey)
        {
            output << " PRIMARY_KEY";
        }

        if (column.notNull)
        {
            output << " NOT_NULL";
        }

        if (column.unique)
        {
            output << " UNIQUE";
        }

        if (column.hasCheck)
        {
            output << " CHECK " << column.checkCondition;
        }

        if (column.isForeignKey)
        {
            output << " REFERENCES "
                   << column.referencedTable << " "
                   << column.referencedColumn;
        }

        output << "\n";
    }

    output << "END_TABLE\n";

    output.close();

    return true;
}

// SHOW table
vector<string> StorageManager::getTables(const string &database) const
{

    vector<string> tables;

    string path = "data/" + database + ".db";

    ifstream file(path);

    if (!file.is_open())
    {
        return tables;
    }

    string line;

    while (getline(file, line))
    {

        if (line.find("TABLE ") == 0)
        {

            string tableName = line.substr(6);

            tables.push_back(tableName);
        }
    }

    file.close();

    return tables;
}

// DESCRIBE table

vector<Column> StorageManager::getTableColumns(
    const string &database,
    const string &tableName) const
{
    vector<Column> columns;

    string path = "data/" + database + ".db";

    ifstream file(path);

    if (!file.is_open())
    {
        return columns;
    }

    string line;
    bool tableFound = false;

    while (getline(file, line))
    {
        // Check whether this is the requested table
        if (line == "TABLE " + tableName)
        {
            tableFound = true;
            continue;
        }

        // Stop reading when the table definition ends
        if (tableFound && line == "END_TABLE")
        {
            break;
        }

        // Extract column information
        if (tableFound && line.find("COLUMN ") == 0)
        {
            stringstream ss(line.substr(7));

            Column column;
            string constraint;

            if (ss >> column.name >> column.type)
            {
                // Read column constraints
                while (ss >> constraint)
                {
                    if (constraint == "PRIMARY_KEY")
                    {
                        column.primaryKey = true;
                        column.notNull = true;
                    }
                    else if (constraint == "NOT_NULL")
                    {
                        column.notNull = true;
                    }
                    else if (constraint == "UNIQUE")
                    {
                        column.unique = true;
                    }
                    else if (constraint == "CHECK")
                    {
                        column.hasCheck = true;

                        // Read the complete CHECK condition
                        getline(ss, column.checkCondition);

                        // Remove leading spaces
                        size_t first = column.checkCondition.find_first_not_of(" \t");

                        if (first != string::npos)
                        {
                            column.checkCondition =
                                column.checkCondition.substr(first);
                        }
                        else
                        {
                            column.checkCondition.clear();
                        }

                        break;
                    }
                    else if (constraint == "REFERENCES")
                    {
                        column.isForeignKey = true;

                        if (!(ss >> column.referencedTable >>
                              column.referencedColumn))
                        {
                            file.close();
                            return {};
                        }
                    }
                }

                columns.push_back(column);
            }
        }
    }

    file.close();

    return columns;
}

bool StorageManager::insertRow(
    const string &database,
    const string &tableName,
    const vector<string> &values)
{
    string path = "data/" + database + ".db";

    ifstream input(path);
    if (!input.is_open())
    {
        return false;
    }

    vector<string> lines;
    string line;
    bool tableFound = false;
    int dataStart = -1;
    int dataEnd = -1;

    // Read the existing database file.
    while (getline(input, line))
    {
        if (line == "TABLE " + tableName)
        {
            tableFound = true;
        }

        if (line == "DATA " + tableName)
        {
            dataStart = static_cast<int>(lines.size());
        }

        if (line == "END_DATA" && dataStart != -1)
        {
            dataEnd = static_cast<int>(lines.size());
        }

        lines.push_back(line);
    }

    input.close();

    if (!tableFound)
    {
        return false;
    }

    // Retrieve the table schema.
    vector<Column> columns = getTableColumns(database, tableName);

    if (columns.empty() || values.size() != columns.size())
    {
        return false;
    }

    // Validate NOT NULL and PRIMARY KEY constraints.
    vector<vector<string>> existingRows = getRows(database, tableName);

    for (size_t i = 0; i < columns.size(); i++)
    {
        // Enforce NOT NULL
        if (columns[i].notNull && values[i].empty())
            return false;

        // Enforce PRIMARY KEY and UNIQUE
        if (columns[i].primaryKey || (columns[i].unique && !values[i].empty()))
        {
            for (const auto &existingRow : existingRows)
            {
                if (i < existingRow.size() && existingRow[i] == values[i])
                    return false;
            }
        }

        // Enforce CHECK constraint
        if (columns[i].hasCheck && !values[i].empty())
        {
            string condition = columns[i].checkCondition;

            // Identify the comparison operator
            string operators[] = {">=", "<=", "!=", "=", ">", "<"};

            string op;
            size_t operatorPosition = string::npos;

            for (const string &candidate : operators)
            {
                operatorPosition = condition.find(candidate);

                if (operatorPosition != string::npos)
                {
                    op = candidate;
                    break;
                }
            }

            if (operatorPosition == string::npos)
                return false;

            // Extract the expected value
            string left = condition.substr(0, operatorPosition);
            string right = condition.substr(
                operatorPosition + op.length());

            // Remove surrounding spaces
            auto trim = [](string &value)
            {
                size_t first = value.find_first_not_of(" \t");
                size_t last = value.find_last_not_of(" \t");

                if (first == string::npos)
                {
                    value.clear();
                    return;
                }

                value = value.substr(first, last - first + 1);
            };

            trim(left);
            trim(right);

            // Ensure the condition refers to the current column
            if (left != columns[i].name)
                return false;

            // Evaluate numeric CHECK conditions
            try
            {
                size_t actualLength = 0;
                size_t expectedLength = 0;

                long long actualValue = stoll(values[i], &actualLength);
                long long expectedValue = stoll(right, &expectedLength);

                if (actualLength != values[i].length() ||
                    expectedLength != right.length())
                    return false;

                bool checkPassed = false;

                if (op == ">=")
                    checkPassed = actualValue >= expectedValue;
                else if (op == "<=")
                    checkPassed = actualValue <= expectedValue;
                else if (op == "!=")
                    checkPassed = actualValue != expectedValue;
                else if (op == "=")
                    checkPassed = actualValue == expectedValue;
                else if (op == ">")
                    checkPassed = actualValue > expectedValue;
                else if (op == "<")
                    checkPassed = actualValue < expectedValue;

                if (!checkPassed)
                    return false;
            }
            catch (...)
            {
                return false;
            }
        }
    }

    // Enforce FOREIGN KEY constraint
    for (size_t i = 0; i < columns.size(); i++)
    {
        if (!columns[i].isForeignKey || values[i].empty())
        {
            continue;
        }

        // Get columns of the referenced table
        vector<Column> referencedColumns =
            getTableColumns(
                database,
                columns[i].referencedTable);

        // Find the referenced column index
        size_t referencedIndex = referencedColumns.size();

        for (size_t j = 0; j < referencedColumns.size(); j++)
        {
            if (referencedColumns[j].name ==
                columns[i].referencedColumn)
            {
                referencedIndex = j;
                break;
            }
        }

        if (referencedIndex == referencedColumns.size())
        {
            return false;
        }

        // Get existing rows from the referenced table
        vector<vector<string>> referencedRows =
            getRows(database, columns[i].referencedTable);

        bool referenceFound = false;

        // Check whether the referenced value exists
        for (const auto &referencedRow : referencedRows)
        {
            if (referencedIndex < referencedRow.size() &&
                referencedRow[referencedIndex] == values[i])
            {
                referenceFound = true;
                break;
            }
        }

        if (!referenceFound)
        {
            return false;
        }
    }

    // Prepare the row.
    string row;

    for (size_t i = 0; i < values.size(); i++)
    {
        if (values[i].find('|') != string::npos ||
            values[i].find('\n') != string::npos)
        {
            return false;
        }

        if (i > 0)
        {
            row += "|";
        }

        row += values[i];
    }

    // Add a new DATA section if one doesn't exist.
    if (dataStart == -1)
    {
        lines.push_back("DATA " + tableName);
        lines.push_back(row);
        lines.push_back("END_DATA");
    }
    else
    {
        // Ensure the DATA section is properly terminated.
        if (dataEnd == -1)
        {
            return false;
        }

        // Insert the row before END_DATA.
        lines.insert(lines.begin() + dataEnd, row);
    }

    // Write the updated contents to a temporary file.
    string tempPath = path + ".tmp";

    ofstream output(tempPath);
    if (!output.is_open())
    {
        return false;
    }

    for (const string &existingLine : lines)
    {
        output << existingLine << '\n';
    }

    output.close();

    if (!output)
    {
        fs::remove(tempPath);
        return false;
    }

    // Replace the original database file.
    error_code ec;
    fs::copy_file(
        tempPath,
        path,
        fs::copy_options::overwrite_existing,
        ec);

    fs::remove(tempPath);

    return !ec;
}

// SELECT from table
vector<vector<string>> StorageManager::getRows(
    const string &database,
    const string &tableName)
{
    vector<vector<string>> rows;

    string path = "data/" + database + ".db";
    ifstream file(path);

    if (!file.is_open())
    {
        return rows;
    }

    string line;
    bool readingData = false;

    while (getline(file, line))
    {
        if (line == "DATA " + tableName)
        {
            readingData = true;
            continue;
        }

        if (readingData && line == "END_DATA")
        {
            break;
        }

        if (readingData)
        {
            vector<string> row;
            stringstream ss(line);
            string value;

            while (getline(ss, value, '|'))
            {
                row.push_back(value);
            }

            rows.push_back(row);
        }
    }

    file.close();
    return rows;
}

// UPDATE and DELETE a row in a table
bool StorageManager::saveRows(
    const string &database,
    const string &tableName,
    const vector<vector<string>> &rows)
{
    string path = "data/" + database + ".db";

    if (!fs::exists(path))
    {
        return false;
    }

    // Read the existing database file.
    vector<string> lines;
    ifstream inputFile(path);

    if (!inputFile.is_open())
    {
        return false;
    }

    string line;

    while (getline(inputFile, line))
    {
        lines.push_back(line);
    }

    inputFile.close();

    // Find the DATA section of the specified table.
    string dataStart = "DATA " + tableName;

    size_t start = lines.size();
    size_t end = lines.size();

    for (size_t i = 0; i < lines.size(); i++)
    {
        if (lines[i] == dataStart)
        {
            start = i;
            break;
        }
    }

    if (start == lines.size())
    {
        return false;
    }

    for (size_t i = start + 1; i < lines.size(); i++)
    {
        if (lines[i] == "END_DATA")
        {
            end = i;
            break;
        }
    }

    if (end == lines.size())
    {
        return false;
    }

    // Replace the existing records with updated records.
    vector<string> updatedLines;

    for (const auto &row : rows)
    {
        string rowData;

        for (size_t i = 0; i < row.size(); i++)
        {
            if (row[i].find('|') != string::npos ||
                row[i].find('\n') != string::npos)
            {
                return false;
            }

            if (i > 0)
            {
                rowData += "|";
            }

            rowData += row[i];
        }

        updatedLines.push_back(rowData);
    }

    // Replace only the records inside the DATA section.
    lines.erase(lines.begin() + start + 1, lines.begin() + end);

    lines.insert(
        lines.begin() + start + 1,
        updatedLines.begin(),
        updatedLines.end());

    // Write the modified database to a temporary file.
    string tempPath = path + ".tmp";

    ofstream outputFile(tempPath);

    if (!outputFile.is_open())
    {
        return false;
    }

    for (const string &currentLine : lines)
    {
        outputFile << currentLine << '\n';
    }

    outputFile.close();

    if (!outputFile)
    {
        fs::remove(tempPath);
        return false;
    }

    // Replace the original database file.
    try
    {
        fs::copy_file(
            tempPath,
            path,
            fs::copy_options::overwrite_existing);

        fs::remove(tempPath);
    }
    catch (...)
    {
        fs::remove(tempPath);
        return false;
    }

    return true;
}