#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

static const int PRIME_NUMBER = 37;

class Cell : public CellInterface
{

public:
    Cell(SheetInterface &sheet);

    ~Cell();

    struct PositionHasher
    {
        size_t operator()(const Position &pos) const
        {
            return (pos.row + 1) * PRIME_NUMBER + (pos.col + 1) * PRIME_NUMBER * PRIME_NUMBER;
        }
    };

    void Set(std::string text);

    void Clear();

    Value GetValue() const override;

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    std::unordered_set<Position, PositionHasher> GetReferenced() const;

    std::unordered_set<Position, PositionHasher> GetCellsThatRefer() const;

    void RemoveOldDependence(Position pos);

    void AddNewDependence(Position pos);

    void ClearCache();

    bool IsReferenced() const;

private:
    class Impl
    {
    public:
        virtual ~Impl() = default;

        virtual Value GetValue() const = 0;

        virtual std::string GetText() const = 0;

        virtual std::vector<Position> GetReferencedCells() const = 0;

        virtual void ClearCache() = 0;
    };

    class TextImpl : public Impl
    {
    public:
        TextImpl(std::string str);

        Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        void ClearCache() override;

    private:
        std::string value_;
    };

    class FormulaImpl : public Impl
    {
    public:
        FormulaImpl(std::string str, SheetInterface &sheet);

        Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        void ClearCache() override;

    private:
        std::unique_ptr<FormulaInterface> ast_;
        const SheetInterface &sheet_;
        mutable std::optional<Value> cache_value_;
    };

    std::unique_ptr<Impl> impl_;
    SheetInterface &sheet_;
    std::unordered_set<Position, PositionHasher> referenced_;
    std::unordered_set<Position, PositionHasher> cells_that_refer_;
};