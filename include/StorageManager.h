#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <vector>

struct Column
{
    std::string name;
    std::string type;
};

class StorageManager
{
public:
    bool createDatabase(const std::string &name);
    bool databaseExists(const std::string &name);

    std::vector<std::string> getDatabases() const;

    bool createTable(
        const std::string &database,
        const std::string &tableName,
        const std::vector<Column> &columns);

    std::vector<std::string> getTables(const std::string &database) const;

    std::vector<Column> getTableColumns(
        const std::string &database,
        const std::string &tableName) const;

    bool insertRow(
        const std::string &database,
        const std::string &tableName,
        const std::vector<std::string> &values);

    std::vector<std::vector<std::string>> getRows(
        const std::string &database,
        const std::string &tableName);

    bool updateRows(
        const std::string &database,
        const std::string &tableName,
        const std::vector<std::vector<std::string>> &rows);
};

#endif
