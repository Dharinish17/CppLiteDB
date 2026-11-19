# CppLiteDB

**CppLiteDB** is a lightweight, SQLite-inspired relational database engine built from scratch using C++17. It provides a command-line interface for managing databases, creating tables, and performing SQL-like operations without depending on external database libraries.

The project focuses on understanding how relational database systems work internally, including SQL command processing, table management, data storage, and query execution.

## Features

### Database Management
- Create databases using `CREATE DATABASE`.
- Switch between databases using `USE`.
- Display available databases using `SHOW DATABASES`.

### Table Management
- Create tables using `CREATE TABLE`.
- Display tables using `SHOW TABLES`.
- View table structure using `DESCRIBE`.

### Data Manipulation
- Insert records using `INSERT INTO`.
- Retrieve records using `SELECT`.
- Update existing records using `UPDATE`.
- Delete records using `DELETE FROM`.

### Query Operations
- Select specific columns or all columns.
- Filter records using `WHERE`.
- Support comparison operators: `=`, `!=`, `>`, `<`, `>=`, `<=`.
- Combine conditions using `AND` and `OR`.
- Sort records using `ORDER BY`.
- Restrict results using `LIMIT`.
- Retrieve unique records using `DISTINCT`.
- Perform aggregate operations using:
  - `COUNT()`
  - `SUM()`
  - `AVG()`
  - `MIN()`
  - `MAX()`

### Constraints
- `PRIMARY KEY`
- `NOT NULL`
- `UNIQUE`

## Tech Stack

- **Language:** C++
- **Compiler:** GCC / MinGW
- **Storage:** Custom file-based storage using `.db` files
- **Interface:** Command-line interface (CLI)

## Getting Started

### Prerequisites

- GCC compiler with C++17 support.
- Windows, Linux, or another environment supporting GCC and standard C++17.

### Compilation

Clone the repository:

```bash
git clone https://github.com/Dharinish17/CppLiteDB.git
cd CppLiteDB
```

Compile the project:

```bash
g++ -std=c++17 src/*.cpp -I include -o CppLiteDB.exe
```

### Run

On Windows PowerShell:

```powershell
.\CppLiteDB.exe
```

On Linux:

```bash
./CppLiteDB
```

## Usage Examples

The following examples demonstrate the supported SQL-like command syntax.

### 1. Create and Select a Database

```sql
CREATE DATABASE college;
USE college;
```

### 2. Create a Table

```sql
CREATE TABLE students (
    id INT PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    age INT
);
```

### 3. Insert Records

```sql
INSERT INTO students VALUES (1, 'Rahul', 20);
INSERT INTO students VALUES (2, 'Priya', 21);
INSERT INTO students VALUES (3, 'Arjun', 19);
```

### 4. Retrieve Records

```sql
SELECT * FROM students;
```

Retrieve specific columns:

```sql
SELECT name, age FROM students;
```

### 5. Filter Records

```sql
SELECT * FROM students WHERE age > 19;
```

Combine conditions:

```sql
SELECT * FROM students
WHERE age > 18 AND id <= 3;
```

### 6. Sort and Limit Results

```sql
SELECT * FROM students ORDER BY age DESC;
```

```sql
SELECT * FROM students LIMIT 2;
```

### 7. Retrieve Distinct Values

```sql
SELECT DISTINCT age FROM students;
```

### 8. Aggregate Functions

```sql
SELECT COUNT(*) FROM students;
SELECT SUM(age) FROM students;
SELECT AVG(age) FROM students;
SELECT MIN(age) FROM students;
SELECT MAX(age) FROM students;
```

### 9. Update Records

```sql
UPDATE students SET age = 22 WHERE id = 2;
```

### 10. Delete Records

```sql
DELETE FROM students WHERE id = 3;
```

## Storage Mechanism

CppLiteDB uses a custom file-based storage mechanism instead of relying on an existing database engine.

Each database is stored as a `.db` file inside the `data/` directory.

The current storage format is text-based and maintains separate sections for:

- Database metadata and version information.
- Table definitions and column constraints.
- Table records.

Example representation:

```text
CPPLITEDB
VERSION 1

TABLE students
COLUMN id int PRIMARY_KEY NOT_NULL
COLUMN name varchar(50) NOT_NULL
END_TABLE

DATA students
1|rahul|20
2|priya|21
END_DATA
```

## Learning Objectives

This project is intended to explore the internal working of relational database systems through hands-on C++ development.

Key concepts explored include:

- Relational database fundamentals.
- SQL command parsing and execution.
- Data definition and manipulation.
- Schema and constraint management.
- Query filtering, sorting, and aggregation.

---

*CppLiteDB is an educational project developed to understand and implement the fundamental concepts behind relational database management systems from scratch.*