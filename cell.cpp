#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(SheetInterface &sheet)
    : sheet_(sheet)
{
}

Cell::~Cell()
{
    Clear();
}

void Cell::Set(std::string text)
{
    if (text[0] == '=' && text.size() != 1)
    {
        text = text.substr(1);
        impl_ = std::make_unique<FormulaImpl>(std::move(text), sheet_);
        const auto &referenced = impl_->GetReferencedCells();
        for (const auto &cell : referenced)
        {
            referenced_.insert(cell);
        }
    }
    else
    {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Clear()
{
    impl_ = nullptr;
}

Cell::Value Cell::GetValue() const
{
    return impl_->GetValue();
}

std::string Cell::GetText() const
{
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

std::unordered_set<Position, Cell::PositionHasher> Cell::GetReferenced() const
{
    return referenced_;
}

std::unordered_set<Position, Cell::PositionHasher> Cell::GetCellsThatRefer() const
{
    return cells_that_refer_;
}

void Cell::RemoveOldDependence(Position pos)
{
    referenced_.erase(pos);
}

void Cell::AddNewDependence(Position pos)
{
    cells_that_refer_.insert(pos);
}

void Cell::ClearCache()
{
    impl_->ClearCache();
}

bool Cell::IsReferenced() const
{
    return !referenced_.empty();
}

Cell::TextImpl::TextImpl(std::string str)
    : value_(std::move(str))
{
}

Cell::Value Cell::TextImpl::GetValue() const
{
    if (value_[0] == '\'')
    {
        return value_.substr(1);
    }
    return value_;
}

std::string Cell::TextImpl::GetText() const
{
    return value_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const
{
    return std::vector<Position>();
}

void Cell::TextImpl::ClearCache()
{
}

Cell::FormulaImpl::FormulaImpl(std::string str, SheetInterface &sheet)
    : ast_(ParseFormula(std::move(str))), sheet_(sheet)
{
}

Cell::Value Cell::FormulaImpl::GetValue() const
{
    if (cache_value_.has_value())
    {
        return cache_value_.value();
    }
    else
    {
        FormulaInterface::Value value = ast_->Evaluate(sheet_);
        if (std::holds_alternative<double>(value))
        {
            cache_value_ = std::get<double>(value);
        }
        else
        {
            cache_value_ = std::get<FormulaError>(value);
        }
        return cache_value_.value();
    }
}

std::string Cell::FormulaImpl::GetText() const
{
    return '=' + ast_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const
{
    return ast_->GetReferencedCells();
}

void Cell::FormulaImpl::ClearCache()
{
    cache_value_.reset();
}