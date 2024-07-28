#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    GetConcreteCell(pos)->Set(text);
    ResizePrintable(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("#POS!");
    } 
    if (!IsCellInRange(pos) || cells_ptr_table_[pos.row][pos.col] == nullptr) {
        return nullptr;
    }
    return cells_ptr_table_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("#POS!");
    } 
    if (!IsCellInRange(pos) || cells_ptr_table_[pos.row][pos.col] == nullptr) {
        return nullptr;
    }
    return cells_ptr_table_[pos.row][pos.col].get();
}

//инициализирует пустую ячейку если ее нет
const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("#POS!");
    }
    if (!IsCellInRange(pos)) {
        ResizeTable({ pos.row + 1, pos.col + 1 });
    }
    if (cells_ptr_table_[pos.row][pos.col] == nullptr || cells_ptr_table_[pos.row][pos.col].get() == nullptr) {
        cells_ptr_table_[pos.row][pos.col] = std::move(CreateEmptyCell(*this));
    }
    return dynamic_cast<Cell*>(cells_ptr_table_[pos.row][pos.col].get());
}

//инициализирует пустую ячейку если ее нет
Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("#POS!");
    }
    if (!IsCellInRange(pos)) {
        ResizeTable({ pos.row + 1, pos.col + 1 });
    }
    if (cells_ptr_table_[pos.row][pos.col] == nullptr || cells_ptr_table_[pos.row][pos.col].get() == nullptr) {
        cells_ptr_table_[pos.row][pos.col] = std::move(CreateEmptyCell(*this));
    }
    return dynamic_cast<Cell*>(cells_ptr_table_[pos.row][pos.col].get());
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("#POS!");
    } 
    if (!IsCellInRange(pos)) {
        return;
    }
    cells_ptr_table_[pos.row][pos.col] = nullptr;
    ResizePrintable(pos);
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

Size Sheet::GetActualSize() const {
    return actual_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; i++ ) {
        std::string row;
        for (int j = 0; j < printable_size_.cols; j++ ) {
            if (GetCell({i, j}) != nullptr) {
                row += ValueToString(GetCell({i, j})->GetValue());
            }
            row += '\t';
        }
        row.pop_back();
        output << row << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; i++ ) {
        std::string row;
        for (int j = 0; j < printable_size_.cols; j++ ) {
            if (GetCell({i, j}) != nullptr) {
                row += GetCell({i, j})->GetText();
            }
            row += '\t';
        }
        row.pop_back();
        output << row << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::ResizeTable(Size new_size) const {
    Size new_actual_size = actual_size_;
    if (new_actual_size.cols < new_size.cols) {
        new_actual_size.cols = new_size.cols;
        for (auto& col : cells_ptr_table_) {
            col.resize(new_actual_size.cols);
        }
    }
    if (new_actual_size.rows < new_size.rows) {
        cells_ptr_table_.resize(new_size.rows);
        new_actual_size.rows = new_size.rows;
        for (int i = actual_size_.rows; i < new_actual_size.rows; i++) {
            cells_ptr_table_[i].resize(new_actual_size.cols);
        }
    }
    actual_size_ = new_actual_size;
}

void Sheet::ResizePrintable(Position pos) const {
    if (GetCell(pos) == nullptr || dynamic_cast<const Cell*>(GetCell(pos))->IsEmpty()) {
        if (pos.row == printable_size_.rows - 1) {
            for (int i = pos.row; i >=0; i--) {
                if (std::all_of(cells_ptr_table_[i].begin(), cells_ptr_table_[i].end(),
                        [](const CellPtr& cell) { 
                            return cell == nullptr || dynamic_cast<Cell*>(cell.get())->IsEmpty(); 
                        })) 
                {
                    printable_size_.rows--;
                } else {
                    break;
                }
            }
            if (printable_size_.rows == 0) {
                printable_size_.cols = 0;
                return;
            }
        }

        if (pos.col == printable_size_.cols - 1) {
            for (int i = pos.col; i >=0; i--) {
                if (std::all_of(cells_ptr_table_.begin(), cells_ptr_table_.end(), 
                        [i](const std::vector<CellPtr>& row) { 
                            return row[i] == nullptr || dynamic_cast<Cell*>(row[i].get())->IsEmpty();
                        })) 
                {
                    printable_size_.cols--;
                } else {
                    break;
                }
            }
            if (printable_size_.cols == 0) {
                printable_size_.rows = 0;
            }
        }   
    } 
    else if (GetConcreteCell(pos)->IsNoEmpty()) {
        printable_size_.rows = std::max(pos.row + 1, printable_size_.rows);
        printable_size_.cols = std::max(pos.col + 1, printable_size_.cols);
    }
}

bool Sheet::IsCellInRange(Position pos) const {
    return pos.row < actual_size_.rows && pos.col < actual_size_.cols;
}