#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream &operator<<(std::ostream &output, FormulaError fe)
{
    std::string str = std::string{fe.ToString()};
    return output << str;
}

FormulaError::Category FormulaError::GetCategory() const
{
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const
{
    return category_ == rhs.GetCategory();
}

std::string_view FormulaError::ToString() const
{
    using namespace std::string_literals;
    std::string result;
    if (category_ == FormulaError::Category::Ref)
    {
        result = "#REF!"s;
    }
    else if (category_ == FormulaError::Category::Value)
    {
        result = "#VALUE!"s;
    }
    else
    {
        result = "#DIV/0!"s;
    }
    return result;
}

namespace
{
    class Formula : public FormulaInterface
    {
    public:
        explicit Formula(std::string expression)
            : ast_(ParseFormulaAST(expression))
        {
        }

        Value Evaluate(const SheetInterface &sheet) const override
        {
            auto lambda = [&sheet](Position pos)
            {
                auto cell = sheet.GetCell(pos);
                CellInterface::Value value;
                if (cell == nullptr)
                {
                    value = 0.0;
                }
                else
                {
                    value = cell->GetValue();
                }
                return value;
            };
            try
            {
                return ast_.Execute(lambda);
            }
            catch (const FormulaError &e)
            {
                return e;
            }
        }

        std::string
        GetExpression() const override
        {
            std::ostringstream str;
            ast_.PrintFormula(str);
            return str.str();
        }

        std::vector<Position> GetReferencedCells() const override
        {
            const auto &cells = ast_.GetCells();
            std::vector<Position> result;
            std::set<Position> unique_cells;
            for (const auto &cell : cells)
            {
                if (unique_cells.count(cell))
                {
                    continue;
                }
                else
                {
                    unique_cells.insert(cell);
                    result.push_back(cell);
                }
            }
            return result;
        }

    private:
        FormulaAST ast_;
    };
} // namespace

std::unique_ptr<FormulaInterface>
ParseFormula(std::string expression)
{
    return std::make_unique<Formula>(std::move(expression));
}