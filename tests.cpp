#define CATCH_CONFIG_MAIN  /
#include "catch.hpp"       

#include "UniversalTokenizer.h"
#include "MyersDiff.h"
#include <memory>
#include <string>
#include <vector>

TEST_CASE("Character Tokenizer tests", "[tokenizer][character]") {
    auto tokenizer = CreateTokenizer(UniversalTokenizerMode::CHARACTER);
    
    SECTION("Basic encoding and decoding") {
        std::string text = "abc";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokenizer->Decode(tokens) == text);
    }
    
    SECTION("Unicode character handling") {
        std::string text = "привет";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 6);
        // В UTF-8 каждый русский символ занимает 2 байта
        // Если ParserMode::UTF_8, то будет 6 токенов, иначе 12
        REQUIRE(tokenizer->Decode(tokens) == text);
    }
    
    SECTION("Special characters") {
        std::string text = "a\nb\tc";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 5);
        REQUIRE(tokenizer->Decode(tokens) == text);
    }
}

TEST_CASE("Word Tokenizer tests", "[tokenizer][word]") {
    auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
    
    SECTION("Basic word tokenization") {
        std::string text = "hello world";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 3); // "hello", " ", "world"
        REQUIRE(tokenizer->Decode(tokens) == text);
    }
    
    SECTION("Multiple spaces between words") {
        std::string text = "hello  world";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 4);
        REQUIRE(tokenizer->Decode(tokens) == text);
    }
    
    SECTION("Punctuation handling") {
        std::string text = "hello, world!";
        auto tokens = tokenizer->Encode(text);

        REQUIRE(tokens.size() == 3);
        REQUIRE(tokenizer->Decode(tokens) == text);
    }
}

TEST_CASE("Whitespace Tokenizer tests", "[tokenizer][whitespace]") {
    auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WHITESPACE);
    
    SECTION("Basic whitespace tokenization") {
        std::string text = "hello world";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 2); // "hello", "world"
        REQUIRE(tokenizer->Decode(tokens) == "hello world");
    }
    
    SECTION("Multiple whitespace handling") {
        std::string text = "hello  world\ttest";
        auto tokens = tokenizer->Encode(text);
        
        REQUIRE(tokens.size() == 3); // "hello", "world", "test"
    }
}

TEST_CASE("Myers Diff tests", "[diff]") {
    SECTION("Identical texts") {
        std::string text1 = "This is a test";
        std::string text2 = "This is a test";
        
        auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff diff(std::move(tokenizer), text1, text2);
        
        REQUIRE(diff.AreTextsIdentical() == true);
        REQUIRE(diff.GetLevenshteinDistance() == 0);
    }
    
    SECTION("Simple insertion") {
        std::string text1 = "This is test";
        std::string text2 = "This is a test";
        
        auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff diff(std::move(tokenizer), text1, text2);
        
        REQUIRE(diff.AreTextsIdentical() == false);
        REQUIRE(diff.GetLevenshteinDistance() == 2);
    }
    
    SECTION("Simple deletion") {
        std::string text1 = "This is a test";
        std::string text2 = "This is test";
        
        auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff diff(std::move(tokenizer), text1, text2);
        
        REQUIRE(diff.AreTextsIdentical() == false);
        REQUIRE(diff.GetLevenshteinDistance() == 2);
    }
    
    SECTION("Simple substitution") {
        std::string text1 = "This is a test";
        std::string text2 = "This is the test";
        
        auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff diff(std::move(tokenizer), text1, text2);
        
        REQUIRE(diff.AreTextsIdentical() == false);
        REQUIRE(diff.GetLevenshteinDistance() == 2); 
    }
    
    SECTION("Multiple changes") {
        std::string text1 = "The quick brown fox jumps over the lazy dog";
        std::string text2 = "A fast brown fox jumps above the sleepy dog";
        
        auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff diff(std::move(tokenizer), text1, text2);
        
        REQUIRE(diff.AreTextsIdentical() == false);
        // Изменения: "The"->"A", "quick"->"fast", "over"->"above", "lazy"->"sleepy"
        // что составляет 8 операций (4 удаления + 4 вставки)
        REQUIRE(diff.GetLevenshteinDistance() == 8);
    }
    
    SECTION("Empty text handling") {
        std::string text1 = "";
        std::string text2 = "This is a test";
        
        auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff diff(std::move(tokenizer), text1, text2);
        
        REQUIRE(diff.AreTextsIdentical() == false);
    }
    
    SECTION("Different tokenization modes") {
        std::string text1 = "abcdef";
        std::string text2 = "abcxef";
        
        // По символам
        auto char_tokenizer = CreateTokenizer(UniversalTokenizerMode::CHARACTER);
        MyersDiff char_diff(std::move(char_tokenizer), text1, text2);
        REQUIRE(char_diff.GetLevenshteinDistance() == 2); // delete 'd', insert 'x'
        
        // По словам
        auto word_tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff word_diff(std::move(word_tokenizer), text1, text2);
        REQUIRE(word_diff.GetLevenshteinDistance() == 2); // delete "abcdef", insert "abcxef"
    }

    SECTION("Different tokenization modes several words") {
        std::string text1 = "abcdef abc abc";
        std::string text2 = "abcxef abc ade";
        
        // По символам
        auto char_tokenizer = CreateTokenizer(UniversalTokenizerMode::CHARACTER);
        MyersDiff char_diff(std::move(char_tokenizer), text1, text2);
        REQUIRE(char_diff.GetLevenshteinDistance() == 6); // delete 'd', insert 'x' + delete 'b', insert 'd' + delete 'c', insert 'e'
        
        // По словам
        auto word_tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
        MyersDiff word_diff(std::move(word_tokenizer), text1, text2);
        REQUIRE(word_diff.GetLevenshteinDistance() == 4); // delete "abcdef", insert "abcxef" + delete "abc", insert "ade"
    }
}

TEST_CASE("Diff format output tests", "[diff][format]") {
    std::string text1 = "line1\nline2\nline3\n";
    std::string text2 = "line1\nmodified line\nline3\n";
    
    auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
    MyersDiff diff(std::move(tokenizer), text1, text2);
    
    SECTION("Unified format") {
        std::string unified_diff = diff.GetDiff(DiffFormat::UNIFIED);
        REQUIRE_FALSE(unified_diff.empty());
        // Проверяем наличие маркеров унифицированного формата
        REQUIRE(unified_diff.find("---") != std::string::npos);
        REQUIRE(unified_diff.find("+++") != std::string::npos);
        REQUIRE(unified_diff.find("@@") != std::string::npos);
    }
    
    SECTION("Context format") {
        std::string context_diff = diff.GetDiff(DiffFormat::CONTEXT);
        REQUIRE_FALSE(context_diff.empty());
        // Проверяем наличие маркеров контекстного формата
        REQUIRE(context_diff.find("***") != std::string::npos);
        REQUIRE(context_diff.find("---") != std::string::npos);
    }
    
    SECTION("Normal format") {
        std::string normal_diff = diff.GetDiff(DiffFormat::NORMAL);
        REQUIRE_FALSE(normal_diff.empty());
        // Проверяем наличие маркеров обычного формата
        REQUIRE(normal_diff.find("<") != std::string::npos);
        REQUIRE(normal_diff.find(">") != std::string::npos);
    }
}

TEST_CASE("BPE Tokenizer tests", "[tokenizer][bpe]") {
    auto tokenizer = std::make_unique<BPETokenizer>(UniversalParserMode::UTF_8);
    
    SECTION("Basic BPE training") {
        std::vector<std::string> corpus = {
            "low", "lowest", "newer", "wider"
        };
        
        // Обучаем токенизатор с небольшим словарем
        tokenizer->Train(corpus, 10);
        
        // Получаем словарь
        auto vocab = tokenizer->GetVocabulary();
        
        // Проверяем, что в словаре есть базовые сущности
        REQUIRE(vocab.find("l") != vocab.end());
        REQUIRE(vocab.find("o") != vocab.end());
        REQUIRE(vocab.find("w") != vocab.end());
        
        // Проверяем кодирование и декодирование
        auto tokens = tokenizer->Encode("low");
        REQUIRE_FALSE(tokens.empty());
        REQUIRE(tokenizer->Decode(tokens) == "low");
    }
    
    SECTION("BPE merge rules") {
        // Добавляем правила слияния напрямую
        std::vector<std::pair<std::string, std::string>> merges = {
            {"l", "o"},
            {"lo", "w"},
            {"n", "e"},
            {"w", "i"}
        };
        
        tokenizer->AddMerges(merges);
        
        // Проверяем, что токенизатор правильно применяет правила слияния
        auto tokens = tokenizer->Encode("low");
        REQUIRE_FALSE(tokens.empty());
        std::string decoded = tokenizer->Decode(tokens);
        REQUIRE(decoded == "low");
    }
}

TEST_CASE("File comparison integration tests", "[integration]") {
    // Создаем временные файлы для тестирования
    {
        std::ofstream file1("test_old.txt");
        file1 << "This is line 1.\nThis is line 2.\nThis is line 3.\n";
        file1.close();
        
        std::ofstream file2("test_new.txt");
        file2 << "This is line 1.\nThis is modified line.\nThis is line 3.\n";
        file2.close();
    }
    
    // Функция чтения файла (копируем из main.cpp)
    auto readFileToString = [](const std::string& fileName) -> std::string {
        std::ifstream file(fileName);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };
    
    std::string text1 = readFileToString("test_old.txt");
    std::string text2 = readFileToString("test_new.txt");
    
    REQUIRE_FALSE(text1.empty());
    REQUIRE_FALSE(text2.empty());
    
    // Сравниваем файлы
    auto tokenizer = CreateTokenizer(UniversalTokenizerMode::WORD);
    MyersDiff diff(std::move(tokenizer), text1, text2);
    
    REQUIRE_FALSE(diff.AreTextsIdentical());
    
    // Проверяем результаты сравнения
    auto edit_script = diff.GetShortestEditScript();
    REQUIRE_FALSE(edit_script.empty());
    
    // Проверяем наличие изменения в выводе diff
    std::string unified_diff = diff.GetDiff(DiffFormat::UNIFIED);
    REQUIRE(unified_diff.find("modified") != std::string::npos);
    
    std::remove("test_old.txt");
    std::remove("test_new.txt");
}
