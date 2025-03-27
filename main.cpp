#include "UniversalTokenizer.h"
#include "MyersDiff.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>

std::string readFileToString(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << fileName << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    std::string oldFileName = "old.txt";
    std::string newFileName = "new.txt";
    
    if (argc > 2) {
        oldFileName = argv[1];
        newFileName = argv[2];
    } else if (argc > 1) {
        std::cout << "Использование: " << argv[0] << " old.txt new.txt" << std::endl;
        std::cout << "Используем файлы по умолчанию: " << oldFileName << " и " << newFileName << std::endl;
    }
    
    std::string text1 = readFileToString(oldFileName);
    std::string text2 = readFileToString(newFileName);

    // Пример текстов для сравнения
    // std::string text1 = "Это первый текст для сравнения.\nОн содержит несколько строк.\nНекоторые будут изменены.";
    // std::string text2 = "Это ввторой текст для сравнения.\nОн содержит несколько строк с изменениями.\nДобавлена новая строка.\nИ еще одна строка.";
    
    if (text1.empty() || text2.empty()) {
        std::cerr << "Ошибка: один из файлов пуст или не существует" << std::endl;
        return 1;
    }
    
    // Создаем токенизатор по строкам (можно выбрать любой другой режим)
    auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
    
    MyersDiff diff(std::move(tokenizer), text1, text2);
    
    if (diff.AreTextsIdentical()) {
        std::cout << "Тексты идентичны" << std::endl;
        return 0;
    }
    
    std::cout << "Расстояние Левенштейна: " << diff.GetLevenshteinDistance() << std::endl;
    
    // Выводим различия в унифицированном формате
    std::cout << "\nУнифицированный формат diff:" << std::endl;
    std::cout << diff.GetDiff(DiffFormat::UNIFIED) << std::endl;
    
    // Выводим различия в контекстном формате
    std::cout << "\nКонтекстный формат diff:" << std::endl;
    std::cout << diff.GetDiff(DiffFormat::CONTEXT) << std::endl;
    
    // Выводим различия в обычном формате
    std::cout << "\nОбычный формат diff:" << std::endl;
    std::cout << diff.GetDiff(DiffFormat::NORMAL) << std::endl;
    
    return 0;
}
