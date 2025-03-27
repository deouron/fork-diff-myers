#pragma once

#include "UniversalTokenizer.h"
#include <vector>
#include <utility>
#include <stdexcept>

class Snake {
public:
    Snake(uint32_t begin_x, uint32_t begin_y, uint32_t width);

    std::pair<uint32_t, uint32_t> Begin() const;
    uint32_t Width() const;
    std::pair<uint32_t, uint32_t> End() const;

private:
    uint32_t begin_x_;
    uint32_t begin_y_;
    uint32_t width_;
};

struct Replacement {
    uint32_t from_left;  // Начальная позиция в исходном тексте
    uint32_t from_right; // Конечная позиция в исходном тексте
    uint32_t to_left;    // Начальная позиция в новом тексте
    uint32_t to_right;   // Конечная позиция в новом тексте
};

using EditScript = std::vector<Replacement>;

enum class DiffFormat {
    UNIFIED,  // Унифицированный формат (как в `diff -u`)
    CONTEXT,  // Контекстный формат (как в `diff -c`)
    NORMAL    // Обычный формат (как в `diff`)
};

class MyersDiff {
public:

    MyersDiff(std::unique_ptr<UniversalTokenizer> tokenizer, 
              const std::string& text1, 
              const std::string& text2);
    
    std::vector<TokenId> GetLargestCommonSubsequence() const;
    EditScript GetShortestEditScript() const;
    std::string GetDiff(DiffFormat format = DiffFormat::UNIFIED, int context_size = 3) const;
    
    int GetLevenshteinDistance() const;
    
    bool AreTextsIdentical() const;

private:
    std::pair<uint32_t, Snake> GetMiddleSnake(uint32_t from_left, uint32_t from_right, 
                                          uint32_t to_left, uint32_t to_right) const;
    
    void GetSnakeDecomposition(uint32_t from_left, uint32_t from_right, 
                              uint32_t to_left, uint32_t to_right,
                              std::vector<int32_t>& from_snake,
                              std::vector<int32_t>& to_snake, 
                              uint32_t& current_snake) const;
    
    std::string FormatUnifiedDiff(const EditScript& script, int context_size) const;
    std::string FormatContextDiff(const EditScript& script, int context_size) const;
    std::string FormatNormalDiff(const EditScript& script) const;

    std::unique_ptr<UniversalTokenizer> tokenizer_;
    
    std::vector<TokenId> from_tokens_;
    std::vector<TokenId> to_tokens_;
};
