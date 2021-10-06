#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <set>
#include <unordered_map>

class Sheet : public SheetInterface
{
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface *GetCell(Position pos) const override;

    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;

    void PrintTexts(std::ostream &output) const override;

private:
    std::unordered_map<int, std::unordered_map<int, std::unique_ptr<Cell>>> sheet_;
    Size printable_size_;

    void EnlargeSheet(const Position &pos);

    void ReduceSheet(const Position &pos);

    std::string GetStringFromValue(const CellInterface::Value &value) const;

    void CheckCyclicalDependence(const std::unordered_set<Position, Cell::PositionHasher> &referenced_cells, const Position &root, std::set<Position> &visited_cells) const;

    void ClearCache(const std::unordered_set<Position, Cell::PositionHasher> &cells_that_refer);

    void RemoveOldDependences(const std::unordered_set<Position, Cell::PositionHasher> &cells_that_refer, const Position &pos);

    void AddNewDependences(const std::unordered_set<Position, Cell::PositionHasher> &cells_that_refer, const Position &pos);
};