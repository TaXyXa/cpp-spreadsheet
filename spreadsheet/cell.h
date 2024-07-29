#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <functional>
#include <unordered_set>

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(const std::string& text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    double GetCache();
    void ClearCache();

    void AddObserver (Cell* cell);
    void RemoveObserver (Cell* cell);

    void CheckCircular (std::vector<const Cell*> vector_of_cells) const;

    bool IsReferenced() const;
    bool IsEmpty() const;
    bool IsNoEmpty() const;

private:

    class Impl {
    public:
        virtual void Set(const std::string& text) = 0;
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;
    };

    class EmptyImpl : public Impl {
    public:
        void Set(const std::string& text) override;
        Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:
        void Set(const std::string& text) override;
        Value GetValue() const override;
        std::string GetText() const override;
    private:   
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(Cell* parent_cell, SheetInterface* sheet);
        
        void Set(const std::string& text) override;
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        ~FormulaImpl();
    private:
        std::vector<Position> cells_;
        std::shared_ptr<FormulaInterface> formula_;
        Cell* parent_cell_;
        SheetInterface* sheet_;
    };

    std::unique_ptr<Impl> impl_;
    SheetInterface* sheet_;
    std::optional<double> cache_;
    std::unordered_set<Cell*> observers_;
    
};

std::string ValueToString(Cell::Value value);

std::unique_ptr<CellInterface> CreateEmptyCell(SheetInterface& sheet);
std::unique_ptr<CellInterface> CreateEmptyCell(const SheetInterface& sheet);
