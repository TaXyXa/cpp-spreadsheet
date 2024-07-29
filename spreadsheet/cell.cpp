#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(SheetInterface& sheet)
:impl_ (std::make_unique<EmptyImpl>()), sheet_(&sheet)
{

}

Cell::~Cell() {
    Clear();
}

void Cell::Set(const std::string& text) {
    if (text.empty()) {
        Clear();
    } else if (text[0] == FORMULA_SIGN) {
        FormulaImpl impl(this, sheet_);
        impl.Set(text.substr(1));
        std::vector<Cell*> vector_of_cells_ptr;
        for (auto& pos : impl.GetReferencedCells()) {
            Cell* cell_ptr = dynamic_cast<Sheet*>(sheet_)->GetConcreteCell(pos);
            vector_of_cells_ptr.push_back(cell_ptr);
        }
        
        CheckCircular({vector_of_cells_ptr.begin(), vector_of_cells_ptr.end()});
        
        for (Cell* ptr_of_cell : vector_of_cells_ptr) {
            ptr_of_cell->AddObserver(this);
        }
        impl_ = std::make_unique<FormulaImpl>(impl);
    } else {
        TextImpl impl;
        impl.Set(text);
        impl_ = std::make_unique<TextImpl>(impl);
    }
}
//дак я там и использую эту функцию
void Cell::Clear() {
    ClearCache();
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

double Cell::GetCache() {
    if (cache_.has_value()) {
        return cache_.value();
    }

    if (IsEmpty()) {
        cache_ = 0;
        return cache_.value();
    }

    CellInterface::Value value = impl_->GetValue();

    if (std::holds_alternative<std::string>(value)) {
        size_t pos;
        try {
            cache_ = std::stod(std::get<0>(value), &pos);
            if (pos != std::get<0>(value).length()) {
                throw FormulaError (FormulaError::Category::Value);
            }
        } 
        catch (std::invalid_argument& err) {
            throw FormulaError (FormulaError::Category::Value);
        }
    }
    if (std::holds_alternative<double>(value)) {
        cache_ = std::get<1>(value);
    }
    if (std::holds_alternative<FormulaError>(value)) {
        throw std::get<2>(value);
    }
    
    return cache_.value();
}

void Cell::ClearCache() {
    cache_.reset();
    for (Cell* cell_ptr : observers_) {
        cell_ptr->ClearCache();
    }
}

void Cell::CheckCircular (std::vector<const Cell*> vector_of_cells) const {
    for (const Cell* other_cell : vector_of_cells) {
        if (other_cell == this) {
            throw CircularDependencyException("Circular reference");
        }
        if (other_cell->IsReferenced()) {
            vector_of_cells.push_back(this);
            other_cell->CheckCircular(vector_of_cells);
        }
    }
}

void Cell::AddObserver (Cell* cell) {
    observers_.insert(cell);
}

void Cell::RemoveObserver (Cell* cell) {
    auto iter = observers_.find(cell);
    if (iter != observers_.end()) {
        observers_.erase(iter);
    }
}

bool Cell::IsReferenced() const {
    return (dynamic_cast<FormulaImpl*>(impl_.get()) != nullptr) 
            && !impl_.get()->GetReferencedCells().empty();
}

bool Cell::IsEmpty() const {
    return (dynamic_cast<EmptyImpl*>(impl_.get()) != nullptr);
}

bool Cell::IsNoEmpty() const {
    return !IsEmpty();
}

std::string ValueToString(Cell::Value value) {
    if (std::holds_alternative<double>(value)) {
        std::string str = std::to_string(std::get<double>(value));
        str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
        str.erase ( str.find_last_not_of('.') + 1, std::string::npos );
        return str;
    } else if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    } else {
        return std::string(std::get<FormulaError>(value).ToString());
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

void Cell::EmptyImpl::Set(const std::string& text)  {}

CellInterface::Value Cell::EmptyImpl::GetValue() const  {
    return "";
}
        
std::string Cell::EmptyImpl::GetText() const  {
    return "";
}

void Cell::TextImpl::Set(const std::string& text)  {
    text_ = text;
}

CellInterface::Value Cell::TextImpl::GetValue() const  {
    if (text_[0] == ESCAPE_SIGN) {
        return text_.substr(1, text_.size());
    }
    return GetText();
}

std::string Cell::TextImpl::GetText() const  {
    return text_;
}

Cell::FormulaImpl::FormulaImpl(Cell* parent_cell, SheetInterface* sheet) 
    :formula_(nullptr), parent_cell_(parent_cell), sheet_(sheet)
{}
        
void Cell::FormulaImpl::Set(const std::string& text)  {
    formula_ = ParseFormula(text);
    cells_ = formula_.get()->GetReferencedCells();
}

CellInterface::Value Cell::FormulaImpl::GetValue() const  {
    FormulaInterface::Value ans = formula_->Evaluate(*dynamic_cast<Sheet*>(sheet_));
    if (std::holds_alternative<double>(ans)) {
        return std::get<0>(ans);
    } else {
        return std::get<1>(ans);
    }
}

std::string Cell::FormulaImpl::GetText() const  {
    return FORMULA_SIGN + formula_->GetExpression();
}

Cell::FormulaImpl::~FormulaImpl() {
    for (auto& pos_of_cell : cells_) {
        dynamic_cast<Sheet*>(sheet_)->GetConcreteCell(pos_of_cell)->RemoveObserver(parent_cell_);
    }
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return cells_;
}

std::unique_ptr<CellInterface> CreateEmptyCell(SheetInterface& sheet) {
    return std::make_unique<Cell>(sheet);
}

std::unique_ptr<CellInterface> CreateEmptyCell(const SheetInterface& sheet) {
    return std::make_unique<Cell>(const_cast<SheetInterface&>(sheet));
}


