#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>

using namespace std::literals;

Sheet::~Sheet()
{
}

void Sheet::SetCell(Position pos, std::string text)
{
    if (pos.IsValid())
    {
        Cell cell(*this);
        cell.Set(text);
        std::set<Position> visited_cells;
        CheckCyclicalDependence(cell.GetReferenced(), pos, visited_cells);
        std::unordered_set<Position, Cell::PositionHasher> old_referenced_cells;
        if (GetCell(pos) == nullptr)
        {
            EnlargeSheet(pos);
            sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
        }
        else
        {
            old_referenced_cells = sheet_.at(pos.row).at(pos.col)->GetReferenced();
            sheet_.at(pos.row).at(pos.col)->Clear();
        }
        sheet_.at(pos.row).at(pos.col)->Set(text);
        RemoveOldDependences(old_referenced_cells, pos);
        AddNewDependences(sheet_.at(pos.row).at(pos.col)->GetReferenced(), pos);
        ClearCache(sheet_.at(pos.row).at(pos.col)->GetCellsThatRefer());
    }
    else
    {
        throw InvalidPositionException("Invalid Position Exception"s);
    }
}

const CellInterface *Sheet::GetCell(Position pos) const
{
    if (pos.IsValid())
    {
        if (sheet_.count(pos.row) && sheet_.at(pos.row).count(pos.col))
        {
            return sheet_.at(pos.row).at(pos.col).get();
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        throw InvalidPositionException("Invalid Position Exception"s);
    }
}

CellInterface *Sheet::GetCell(Position pos)
{
    if (pos.IsValid())
    {
        if (sheet_.count(pos.row) && sheet_.at(pos.row).count(pos.col))
        {
            return sheet_.at(pos.row).at(pos.col).get();
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        throw InvalidPositionException("Invalid Position Exception"s);
    }
}

void Sheet::ClearCell(Position pos)
{
    if (pos.IsValid())
    {
        if (sheet_.count(pos.row) && sheet_.at(pos.row).count(pos.col))
        {
            sheet_.at(pos.row).at(pos.col) = nullptr;
            ReduceSheet(pos);
        }
    }
    else
    {
        throw InvalidPositionException("Invalid Position Exception"s);
    }
}

Size Sheet::GetPrintableSize() const
{
    return printable_size_;
}

std::string Sheet::GetStringFromValue(const CellInterface::Value &value) const
{
    std::ostringstream text;
    if (std::holds_alternative<std::string>(value))
    {
        text << std::get<std::string>(value);
    }
    else if (std::holds_alternative<double>(value))
    {
        text << std::get<double>(value);
    }
    else
    {
        text << std::get<FormulaError>(value);
    }
    return text.str();
}

void Sheet::PrintValues(std::ostream &output) const
{
    if (printable_size_.cols != 0 && printable_size_.rows != 0)
    {
        std::string result;
        for (int i = 0; i < printable_size_.rows; ++i)
        {
            for (int j = 0; j < printable_size_.cols; ++j)
            {
                if (sheet_.count(i) && sheet_.at(i).count(j) && sheet_.at(i).at(j) != nullptr)
                {
                    result += GetStringFromValue(sheet_.at(i).at(j)->GetValue());
                }
                result += '\t';
            }
            result += '\n';
        }
        result.erase(result.size() - 2, 2);
        output << result;
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream &output) const
{
    if (printable_size_.cols != 0 && printable_size_.rows != 0)
    {
        std::string result;
        for (int i = 0; i < printable_size_.rows; ++i)
        {
            for (int j = 0; j < printable_size_.cols; ++j)
            {
                if (sheet_.count(i) && sheet_.at(i).count(j) && sheet_.at(i).at(j) != nullptr)
                {
                    result += sheet_.at(i).at(j)->GetText();
                }
                result += '\t';
            }
            result += '\n';
        }
        result.erase(result.size() - 2, 2);
        output << result;
        output << '\n';
    }
}

void Sheet::EnlargeSheet(const Position &pos)
{
    if (printable_size_.rows <= pos.row)
    {
        printable_size_.rows = pos.row + 1;
    }
    if (printable_size_.cols <= pos.col)
    {
        printable_size_.cols = pos.col + 1;
    }
}

void Sheet::ReduceSheet(const Position &pos)
{
    if (pos.col == printable_size_.cols - 1)
    {
        bool need_decrease = true;
        for (int i = 0; i < printable_size_.rows; ++i)
        {
            if (sheet_.count(i) && sheet_.at(i).count(pos.col) && sheet_.at(i).at(pos.col) != nullptr)
            {
                need_decrease = false;
                break;
            }
        }
        if (need_decrease)
        {
            printable_size_.cols -= 1;
            if (pos.col > 0)
            {
                Position p{pos.row, pos.col - 1};
                ReduceSheet(p);
            }
        }
    }
    if (pos.row == printable_size_.rows - 1)
    {
        bool need_decrease = true;
        for (int i = 0; i < printable_size_.cols; ++i)
        {
            if (sheet_.count(pos.row) && sheet_.at(pos.row).count(i) && sheet_.at(pos.row).at(i) != nullptr)
            {
                need_decrease = false;
                break;
            }
        }
        if (need_decrease)
        {
            printable_size_.rows -= 1;
            if (pos.row > 0)
            {
                Position p{pos.row - 1, pos.col};
                ReduceSheet(p);
            }
        }
    }
}

void Sheet::CheckCyclicalDependence(const std::unordered_set<Position, Cell::PositionHasher> &referenced_cells, const Position &root, std::set<Position> &visited_cells) const
{
    for (const auto &cell : referenced_cells)
    {
        if (cell == root)
        {
            throw CircularDependencyException("Circular Dependency"s);
        }
        if (GetCell(cell) != nullptr && sheet_.at(cell.row).at(cell.col)->IsReferenced() && !visited_cells.count(cell))
        {
            CheckCyclicalDependence(sheet_.at(cell.row).at(cell.col)->GetReferenced(), root, visited_cells);
            visited_cells.insert(cell);
        }
    }
}

void Sheet::ClearCache(const std::unordered_set<Position, Cell::PositionHasher> &cells_that_refer)
{
    for (const auto &cell : cells_that_refer)
    {
        if (GetCell({cell.row, cell.col}) != nullptr)
        {
            sheet_.at(cell.row).at(cell.col)->ClearCache();
            const auto &cells_that_refer = sheet_.at(cell.row).at(cell.col)->GetCellsThatRefer();
            if (!cells_that_refer.empty())
            {
                ClearCache(cells_that_refer);
            }
        }
    }
}

void Sheet::RemoveOldDependences(const std::unordered_set<Position, Cell::PositionHasher> &cells_that_refer, const Position &pos)
{
    for (const auto &cell : cells_that_refer)
    {
        if (GetCell({cell.row, cell.col}) != nullptr)
        {
            sheet_.at(cell.row).at(cell.col)->RemoveOldDependence(pos);
        }
    }
}

void Sheet::AddNewDependences(const std::unordered_set<Position, Cell::PositionHasher> &cells_that_refer, const Position &pos)
{
    for (const auto &cell : cells_that_refer)
    {
        if (GetCell({cell.row, cell.col}) != nullptr)
        {
            sheet_.at(cell.row).at(cell.col)->AddNewDependence(pos);
        }
        else
        {
            sheet_[cell.row][cell.col] = std::make_unique<Cell>(*this);
            sheet_.at(cell.row).at(cell.col)->Set("0"s);
            sheet_.at(cell.row).at(cell.col)->AddNewDependence(pos);
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet()
{
    return std::make_unique<Sheet>();
}