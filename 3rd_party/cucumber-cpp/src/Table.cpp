#include <cucumber-cpp/internal/Table.hpp>

namespace cuke {
namespace internal {

void Table::addColumn(const std::string column) throw (std::runtime_error) {
    if (rows.empty()) {
        columns.push_back(column);
    } else {
        throw std::runtime_error("Cannot alter columns after rows have been added");
    }
}

void Table::addRow(const row_type &row) throw (std::range_error, std::runtime_error) {
    const basic_type::size_type colSize = columns.size();
    if (colSize == 0) {
        throw std::runtime_error("No column defined yet");
    } else if (colSize != row.size()) {
        throw std::range_error("Row size does not match the table column size");
    } else {
        rows.push_back(buildHashRow(row));
    }
}

Table::hash_row_type Table::buildHashRow(const row_type &row) {
    hash_row_type hashRow;
    for (columns_type::size_type i = 0; i < columns.size(); ++i) {
        hashRow[columns[i]] = row[i];
    }
    return hashRow;
}

const Table::hashes_type & Table::hashes() const {
    return rows;
}

}
}
