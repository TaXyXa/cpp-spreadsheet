#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

namespace {
class Formula : public FormulaInterface {
public:
    
    explicit Formula(std::string expression) 
            :ast_(ParseFormulaAST(expression))
        {}

    Value Evaluate(SheetInterface& sheet) const override {
        Value answer;
        try {
            answer = ast_.Execute(&sheet);
        } 
        catch (const FormulaError& error) {
            answer = error;
        } 
        return answer;
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        std::string answer = out.str();
        return answer;
    }

    std::vector<Position> GetReferencedCells() const override {
        const std::forward_list<Position>* cells_ptr = &ast_.GetCells();
        std::vector<Position> cells {cells_ptr->begin(), cells_ptr->end()};
        std::sort(cells.begin(), cells.end());
        auto last = std::unique(cells.begin(), cells.end());
        cells.erase(last, cells.end());
        return cells;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Incorrect formula");
    }
    
}