#include "common.h"
#include "FormulaAST.h"
#include "cell.h"
#include "sheet.h"
#include "test_runner_p.h"

#include <iostream>
#include <string>
#include <string_view>

inline std::ostream &operator<<(std::ostream &output, Position pos)
{
    return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char *str, std::size_t)
{
    return Position::FromString(str);
}

inline std::ostream &operator<<(std::ostream &output, Size size)
{
    return output << "(" << size.rows << ", " << size.cols << ")";
}

std::unique_ptr<Cell> CreateCell(Sheet &sheet, const std::string &str)
{
    auto cell = std::make_unique<Cell>(sheet);
    cell->Set(str);
    return cell;
}

namespace
{

    void TestPositionAndStringConversion()
    {
        auto test_single = [](Position pos, std::string_view str)
        {
            ASSERT_EQUAL(pos.ToString(), str);
            ASSERT_EQUAL(Position::FromString(str), pos);
        };

        for (int i = 0; i < 25; ++i)
        {
            test_single(Position{i, i}, char('A' + i) + std::to_string(i + 1));
        }

        test_single(Position{0, 0}, "A1");
        test_single(Position{0, 1}, "B1");
        test_single(Position{0, 25}, "Z1");
        test_single(Position{0, 26}, "AA1");
        test_single(Position{0, 27}, "AB1");
        test_single(Position{0, 51}, "AZ1");
        test_single(Position{0, 52}, "BA1");
        test_single(Position{0, 53}, "BB1");
        test_single(Position{0, 77}, "BZ1");
        test_single(Position{0, 78}, "CA1");
        test_single(Position{0, 701}, "ZZ1");
        test_single(Position{0, 702}, "AAA1");
        test_single(Position{136, 2}, "C137");
        test_single(Position{Position::MAX_ROWS - 1, Position::MAX_COLS - 1}, "XFD16384");
    }

    void TestPositionToStringInvalid()
    {
        ASSERT_EQUAL((Position::NONE).ToString(), "");
        ASSERT_EQUAL((Position{-10, 0}).ToString(), "");
        ASSERT_EQUAL((Position{1, -3}).ToString(), "");
    }

    void TestStringToPositionInvalid()
    {
        ASSERT(!Position::FromString("").IsValid());
        ASSERT(!Position::FromString("A").IsValid());
        ASSERT(!Position::FromString("1").IsValid());
        ASSERT(!Position::FromString("e2").IsValid());
        ASSERT(!Position::FromString("A0").IsValid());
        ASSERT(!Position::FromString("A-1").IsValid());
        ASSERT(!Position::FromString("A+1").IsValid());
        ASSERT(!Position::FromString("R2D2").IsValid());
        ASSERT(!Position::FromString("C3PO").IsValid());
        ASSERT(!Position::FromString("XFD16385").IsValid());
        ASSERT(!Position::FromString("XFE16384").IsValid());
        ASSERT(!Position::FromString("A1234567890123456789").IsValid());
        ASSERT(!Position::FromString("ABCDEFGHIJKLMNOPQRS8").IsValid());
    }

    void TestCells()
    {
        auto &sheet = dynamic_cast<Sheet &>(*CreateSheet());

        auto simple_text = CreateCell(sheet, "simple_text");
        ASSERT_EQUAL(simple_text->GetText(), "simple_text");
        ASSERT_EQUAL(std::get<std::string>(simple_text->GetValue()), "simple_text");

        auto empty_apostroph = CreateCell(sheet, "'");
        ASSERT_EQUAL(empty_apostroph->GetText(), "'");
        ASSERT_EQUAL(std::get<std::string>(empty_apostroph->GetValue()), "");

        auto apostroph = CreateCell(sheet, "'apostroph");
        ASSERT_EQUAL(apostroph->GetText(), "'apostroph");
        ASSERT_EQUAL(std::get<std::string>(apostroph->GetValue()), "apostroph");

        auto text_formula = CreateCell(sheet, "'=1+2");
        ASSERT_EQUAL(text_formula->GetText(), "'=1+2");
        ASSERT_EQUAL(std::get<std::string>(text_formula->GetValue()), "=1+2");

        auto empty_formula = CreateCell(sheet, "=");
        ASSERT_EQUAL(empty_formula->GetText(), "=");
        ASSERT_EQUAL(std::get<std::string>(empty_formula->GetValue()), "=");

        auto formula = CreateCell(sheet, "=1+2");
        ASSERT_EQUAL(formula->GetText(), "=1+2");
        ASSERT_EQUAL(std::get<double>(formula->GetValue()), 3);

        auto switch_text = CreateCell(sheet, "1+2");
        ASSERT_EQUAL(switch_text->GetText(), "1+2");
        ASSERT_EQUAL(std::get<std::string>(switch_text->GetValue()), "1+2");

        switch_text->Set("=1+2");
        ASSERT_EQUAL(switch_text->GetText(), "=1+2");
        ASSERT_EQUAL(std::get<double>(switch_text->GetValue()), 3);

        switch_text->Set("=1/0");
        ASSERT_EQUAL(switch_text->GetText(), "=1/0");
        std::cout << std::get<FormulaError>(switch_text->GetValue()) << std::endl;
    }

    void TestEmpty()
    {
        auto sheet = CreateSheet();
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));
    }

    void TestInvalidPosition()
    {
        auto sheet = CreateSheet();
        try
        {
            sheet->SetCell(Position{-1, 0}, "");
        }
        catch (const InvalidPositionException &)
        {
        }
        try
        {
            sheet->GetCell(Position{0, -2});
        }
        catch (const InvalidPositionException &)
        {
        }
        try
        {
            sheet->ClearCell(Position{Position::MAX_ROWS, 0});
        }
        catch (const InvalidPositionException &)
        {
        }
    }

    void TestOneCell()
    {
        {
            auto sheet = CreateSheet();
            ASSERT_EQUAL(sheet->GetCell("A1"_pos), nullptr);
            ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=777");
            ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{1, 1}));

            std::ostringstream values;
            sheet->PrintValues(values);
            auto temp = values.str();
            ASSERT_EQUAL(values.str(), "777\n");

            std::ostringstream text;
            sheet->PrintTexts(text);
            auto tmp = text.str();
            ASSERT_EQUAL(text.str(), "=777\n");
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=B1+5");
            ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{1, 1}));

            std::ostringstream values;
            sheet->PrintValues(values);
            auto temp = values.str();
            ASSERT_EQUAL(values.str(), "5\n");

            std::ostringstream text;
            sheet->PrintTexts(text);
            auto tmp = text.str();
            ASSERT_EQUAL(text.str(), "=B1+5\n");
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "");
            ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{1, 1}));

            std::ostringstream values;
            sheet->PrintValues(values);
            auto temp = values.str();
            ASSERT_EQUAL(values.str(), "\n");

            std::ostringstream text;
            sheet->PrintTexts(text);
            auto tmp = text.str();
            ASSERT_EQUAL(text.str(), "\n");
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "777");
            sheet->ClearCell("A1"_pos);
            ASSERT_EQUAL(sheet->GetCell("A1"_pos), nullptr);
            ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}));

            std::ostringstream values;
            sheet->PrintValues(values);
            auto temp = values.str();
            ASSERT_EQUAL(values.str(), "");

            std::ostringstream text;
            sheet->PrintTexts(text);
            auto tmp = text.str();
            ASSERT_EQUAL(text.str(), "");
        }
    }

    void TestSetCellPlainText()
    {
        auto sheet = CreateSheet();

        auto checkCell = [&](Position pos, std::string text)
        {
            sheet->SetCell(pos, text);
            CellInterface *cell = sheet->GetCell(pos);
            ASSERT(cell != nullptr);
            ASSERT_EQUAL(cell->GetText(), text);
            ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text);
        };

        checkCell("A1"_pos, "Hello");
        checkCell("A1"_pos, "World");
        checkCell("B2"_pos, "Purr");
        checkCell("A3"_pos, "Meow");

        const SheetInterface &constSheet = *sheet;
        ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr");

        sheet->SetCell("A3"_pos, "'=escaped");
        CellInterface *cell = sheet->GetCell("A3"_pos);
        ASSERT_EQUAL(cell->GetText(), "'=escaped");
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped");
    }

    void TestClearCell()
    {
        auto sheet = CreateSheet();

        sheet->SetCell("C2"_pos, "Me gusta");
        sheet->ClearCell("C2"_pos);
        ASSERT(sheet->GetCell("C2"_pos) == nullptr);

        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("J10"_pos);
    }
    void TestPrint()
    {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "meow");
        sheet->SetCell("B2"_pos, "=1+2");
        sheet->SetCell("A1"_pos, "=1/0");

        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{2, 2}));

        std::ostringstream texts;
        sheet->PrintTexts(texts);
        ASSERT_EQUAL(texts.str(), "=1/0\t\t\nmeow\t=1+2\n");

        std::ostringstream values;
        sheet->PrintValues(values);
        auto temp = values.str();
        ASSERT_EQUAL(values.str(), "#DIV/0!\t\t\nmeow\t3\n");

        sheet->ClearCell("B2"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{2, 1}));
    }

    void TestRefCells()
    {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=1");
            sheet->SetCell("A3"_pos, "=2");
            sheet->SetCell("B5"_pos, "=C2+A3");
            ASSERT(std::get<double>(sheet->GetCell("B5"_pos)->GetValue()) == 3.0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=1");
            sheet->SetCell("A3"_pos, "2"); // A3 - ???????????????? ?????????? "2" => 2.0
            sheet->SetCell("B5"_pos, "=C2+A3");
            ASSERT(std::get<double>(sheet->GetCell("B5"_pos)->GetValue()) == 3.0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=11");
            sheet->SetCell("A3"_pos, ""); // A3 - ???????????????? ???????????? ???????????? "" => 0.0
            sheet->SetCell("B5"_pos, "=C2+A3");
            ASSERT(std::get<double>(sheet->GetCell("B5"_pos)->GetValue()) == 11.0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=1+A2"); // A2 - ????????????
            sheet->SetCell("C2"_pos, "=10+A1");
            sheet->SetCell("B5"_pos, "=C2+A3"); // A3 - ???????????? ???????????? => 0.0
            ASSERT(std::get<double>(sheet->GetCell("B5"_pos)->GetValue()) == 11.0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=7");
            sheet->SetCell("A3"_pos, "=0");
            sheet->SetCell("B5"_pos, "=C2/A3"); // ?????????????? ???? 0
            ASSERT(std::get<FormulaError>(sheet->GetCell("B5"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=7");
            sheet->SetCell("B5"_pos, "=C2/A3"); // A3 - ???????????? ???????????? => 0.0 => ?????????????? ???? 0
            ASSERT(std::get<FormulaError>(sheet->GetCell("B5"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=7");
            sheet->SetCell("A3"_pos, "");       // A3 - ???????????????? ???????????? ???????????? "" => 0.0
            sheet->SetCell("B5"_pos, "=C2/A3"); // ?????????????? ???? 0
            ASSERT(std::get<FormulaError>(sheet->GetCell("B5"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=7");
            sheet->SetCell("A3"_pos, "Text");   // A3 - ???????????????? ?????????? (???? ??????????)
            sheet->SetCell("B5"_pos, "=C2+A3"); // ???????????? #VALUE!
            ASSERT(std::get<FormulaError>(sheet->GetCell("B5"_pos)->GetValue()).GetCategory() == FormulaError::Category::Value);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=7");
            sheet->SetCell("A3"_pos, "=0");
            sheet->SetCell("B5"_pos, "=C2/A3"); // ?????????????? ???? 0
            sheet->SetCell("B6"_pos, "=B5+1");  // ?????????????? ???? 0 ????????????????????????????????
            ASSERT(std::get<FormulaError>(sheet->GetCell("B6"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0 ||
                   std::get<FormulaError>(sheet->GetCell("B6"_pos)->GetValue()).GetCategory() == FormulaError::Category::Value);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("C2"_pos, "=7");
            sheet->SetCell("A3"_pos, "Text");   // A3 - ???????????????? ?????????? (???? ??????????)
            sheet->SetCell("B5"_pos, "=C2+A3"); // ???????????? #VALUE!
            sheet->SetCell("B6"_pos, "=B5+1");  // #VALUE! ????????????????????????????????
            ASSERT(std::get<FormulaError>(sheet->GetCell("B6"_pos)->GetValue()).GetCategory() == FormulaError::Category::Value);
        }
    }

    void TestExceptions()
    {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=1");
            sheet->SetCell("B1"_pos, "=22");
            try
            {
                sheet->SetCell("B1"_pos, "=A1+*");
                assert(false);
            }
            catch (std::exception &ex)
            { // ParsingError, FormulaException, FormulaError
                std::cout << ex.what() << std::endl;
            }
            ASSERT(std::get<double>(sheet->GetCell("B1"_pos)->GetValue()) == 22.0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=1/0");
            ASSERT(std::get<FormulaError>(sheet->GetCell("A1"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=1");
            sheet->SetCell("A2"_pos, "=2");
            sheet->SetCell("A3"_pos, "=A1/A2");
            sheet->ClearCell("A2"_pos);
            ASSERT(std::get<FormulaError>(sheet->GetCell("A3"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=3");
            sheet->SetCell("A2"_pos, "=3");
            sheet->SetCell("A3"_pos, "=1/(A1-A2)");
            ASSERT(std::get<FormulaError>(sheet->GetCell("A3"_pos)->GetValue()).GetCategory() == FormulaError::Category::Div0);
        }
    }

    void TestCircularDependency()
    {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=10");
            try
            {
                sheet->SetCell("A1"_pos, "=A1");
                assert(false);
            }
            catch (CircularDependencyException &ex)
            {
                std::cout << ex.what() << std::endl;
            }
        }
        {
            auto sheet = CreateSheet();
            try
            {
                sheet->SetCell("A1"_pos, "=A1");
                assert(false);
            }
            catch (CircularDependencyException &ex)
            {
                std::cout << ex.what() << std::endl;
            }
        }
        {
            auto sheet = CreateSheet();
            try
            {
                sheet->SetCell("A1"_pos, "=C1+A1");
                assert(false);
            }
            catch (CircularDependencyException &ex)
            {
                std::cout << ex.what() << std::endl;
            }
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A2"_pos, "=3");
            sheet->SetCell("C2"_pos, "=A3/A2");
            sheet->SetCell("C4"_pos, "=C2+8");
            try
            {
                sheet->SetCell("A3"_pos, "=C4-1");
                assert(false);
            }
            catch (CircularDependencyException &ex)
            {
                std::cout << ex.what() << std::endl;
            }
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A7"_pos, "=7");
            sheet->SetCell("A1"_pos, "=A7+A3");
            sheet->SetCell("B2"_pos, "=A7/A1");
            sheet->SetCell("C3"_pos, "=B2+8");
            try
            {
                sheet->SetCell("A3"_pos, "=C3-1");
                assert(false);
            }
            catch (CircularDependencyException &ex)
            {
                std::cout << ex.what() << std::endl;
            }
        }
    }

    void TestCache()
    {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=10");
            sheet->SetCell("B2"_pos, "=A1+10");
            sheet->SetCell("A1"_pos, "=100");
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("B3"_pos, "=B2-1");
            sheet->SetCell("B2"_pos, "=A1+10");
            sheet->SetCell("A1"_pos, "=100");
            ASSERT_EQUAL(std::get<double>(sheet->GetCell("B3"_pos)->GetValue()), 109);
            sheet->SetCell("A1"_pos, "=101");
            ASSERT_EQUAL(std::get<double>(sheet->GetCell("B3"_pos)->GetValue()), 110);
        }
    }

} // namespace

int main()
{
    TestRunner tr;
    RUN_TEST(tr, TestPositionAndStringConversion);
    RUN_TEST(tr, TestPositionToStringInvalid);
    RUN_TEST(tr, TestStringToPositionInvalid);
    RUN_TEST(tr, TestCells);
    RUN_TEST(tr, TestEmpty);
    RUN_TEST(tr, TestOneCell);
    RUN_TEST(tr, TestInvalidPosition);
    RUN_TEST(tr, TestSetCellPlainText);
    RUN_TEST(tr, TestClearCell);
    RUN_TEST(tr, TestPrint);

    RUN_TEST(tr, TestRefCells);
    RUN_TEST(tr, TestExceptions);
    RUN_TEST(tr, TestCircularDependency);
    RUN_TEST(tr, TestCache);
    return 0;
}
