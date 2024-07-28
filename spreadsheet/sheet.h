#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    using CellPtr = std::unique_ptr<CellInterface>;

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);


    bool IsCellInRange(Position pos) const;

private:
    //void MaybeIncreaseSizeToIncludePosition(Position pos);
    void PrintCells(std::ostream& output,
                    const std::function<void(const CellInterface&)>& printCell) const;
    Size GetActualSize() const;
    void ResizeTable(Size new_size) const;
    void ResizePrintable(Position pos) const;

    mutable std::vector<std::vector<CellPtr>> cells_ptr_table_;
    mutable Size actual_size_;
    mutable Size printable_size_;
};
