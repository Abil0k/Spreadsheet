# Spreadsheet
Упрощенный аналог существующих таблиц(Microsoft Excel или Google Sheets). В ячейках таблицы могут быть текст или формулы. Формулы, как и в существующих решениях, могут содержать индексы ячеек. Формулы разбираются при помощи специальной программы ANTLR, которая генерирует код лексического и синтаксического анализаторов, а также код для обхода дерева разбора на С++.

# Требования

   * C++17 и выше
    
   * Java SE Runtime Environment 8
    
   * ANTLR

# Порядок сборки

   1. Установить Java SE Runtime Environment 8.
   
   2. Установить ANTLR - ANother Tool for Language Recognition (документация https://github.com/antlr/antlr4/blob/master/doc/getting-started.md).
   
   3. Проверить в файлах FindANTLR.cmake и CMakeLists.txt название файла antlr-X.X.X-complete.jar на корректность версии. Вместо "X.X.X" указать свою версию antlr.
   
   4. Создать папку с названием "antlr4_runtime" без кавычек и скачайть в неё файлы [C++ Target](https://www.antlr.org/download.html).
   
   5.  Запустить cmake build с CMakeLists.txt.
