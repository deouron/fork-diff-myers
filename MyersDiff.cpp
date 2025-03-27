#include "MyersDiff.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <cmath>

Snake::Snake(uint32_t begin_x, uint32_t begin_y, uint32_t width)
    : begin_x_(begin_x), begin_y_(begin_y), width_(width) {
}

std::pair<uint32_t, uint32_t> Snake::Begin() const {
    return {begin_x_, begin_y_};
}

uint32_t Snake::Width() const {
    return width_;
}

std::pair<uint32_t, uint32_t> Snake::End() const {
    return {begin_x_ + width_, begin_y_ + width_};
}

MyersDiff::MyersDiff(std::unique_ptr<UniversalTokenizer> tokenizer, 
                     const std::string& text1, 
                     const std::string& text2)
    : tokenizer_(std::move(tokenizer)) {
    
    from_tokens_ = tokenizer_->Encode(text1);
    to_tokens_ = tokenizer_->Encode(text2);
}

std::pair<uint32_t, Snake> MyersDiff::GetMiddleSnake(uint32_t from_left, uint32_t from_right, 
                                                  uint32_t to_left, uint32_t to_right) const {
    uint32_t from_size = from_right - from_left;
    uint32_t to_size = to_right - to_left;

    uint32_t total_size = from_size + to_size;
    int32_t delta = from_size - to_size;
    uint32_t offset = total_size + 1 + std::abs(delta);
    bool is_odd = total_size & 1;

    std::vector<uint32_t> max_direct_path(offset * 2);
    std::vector<int32_t> max_reversed_path(offset * 2, from_size + 1);

    for (uint32_t script_size = 0; script_size <= (total_size + 1) / 2; ++script_size) {
        uint32_t lowest_diag = offset - script_size;
        uint32_t highest_diag = offset + script_size;
        
        // Прямой проход
        for (uint32_t diagonal = lowest_diag; diagonal <= highest_diag; diagonal += 2) {
            uint32_t from_id;
            if (diagonal == lowest_diag ||
                (diagonal != highest_diag &&
                 max_direct_path[diagonal - 1] < max_direct_path[diagonal + 1])) {
                from_id = max_direct_path[diagonal + 1];
            } else {
                from_id = max_direct_path[diagonal - 1] + 1;
            }

            uint32_t to_id = from_id + offset - diagonal + to_left;
            from_id += from_left;
            uint32_t snake_length = 0;
            
            // Расширяем змейку пока есть совпадения
            while (from_id < from_right && to_id < to_right && 
                   from_tokens_[from_id] == to_tokens_[to_id]) {
                ++from_id;
                ++to_id;
                ++snake_length;
            }
            
            max_direct_path[diagonal] = from_id - from_left;

            // Проверяем, нашли ли мы среднюю змейку
            if (is_odd && diagonal >= lowest_diag + delta + 1 &&
                diagonal <= highest_diag + delta - 1 &&
                static_cast<int32_t>(max_direct_path[diagonal]) >= max_reversed_path[diagonal]) {
                return {script_size * 2 - 1,
                        {from_id - snake_length, to_id - snake_length, snake_length}};
            }
        }

        // Обратный проход
        lowest_diag += delta;
        highest_diag += delta;
        for (uint32_t diagonal = lowest_diag; diagonal <= highest_diag; diagonal += 2) {
            int32_t from_id;
            if (diagonal == lowest_diag ||
                (diagonal != highest_diag &&
                 max_reversed_path[diagonal + 1] <= max_reversed_path[diagonal - 1])) {
                from_id = max_reversed_path[diagonal + 1] - 1;
            } else {
                from_id = max_reversed_path[diagonal - 1];
            }

            int32_t to_id = from_id + offset - diagonal + to_left;
            from_id += from_left;
            uint32_t snake_length = 0;
            
            // Расширяем змейку в обратном направлении
            while (from_id > static_cast<int32_t>(from_left) &&
                   to_id > static_cast<int32_t>(to_left) && 
                   from_tokens_[from_id - 1] == to_tokens_[to_id - 1]) {
                --from_id;
                --to_id;
                ++snake_length;
            }
            
            max_reversed_path[diagonal] = from_id - from_left;

            // Проверяем, нашли ли мы среднюю змейку
            if (!is_odd && diagonal >= lowest_diag - delta && diagonal <= highest_diag - delta &&
                static_cast<int32_t>(max_direct_path[diagonal]) >= max_reversed_path[diagonal]) {
                return {
                    script_size * 2,
                    {static_cast<uint32_t>(from_id), static_cast<uint32_t>(to_id), snake_length}};
            }
        }
    }
    
    throw std::logic_error("SES не найден");
}

void MyersDiff::GetSnakeDecomposition(uint32_t from_left, uint32_t from_right, 
                                     uint32_t to_left, uint32_t to_right,
                                     std::vector<int32_t>& from_snake,
                                     std::vector<int32_t>& to_snake, 
                                     uint32_t& current_snake) const {
    auto [ses_size, snake] = GetMiddleSnake(from_left, from_right, to_left, to_right);
    
    if (ses_size > 1) {
        auto [from_begin, to_begin] = snake.Begin();
        auto [from_end, to_end] = snake.End();
        
        // Обрабатываем левую часть
        GetSnakeDecomposition(from_left, from_begin, to_left, to_begin, from_snake,
                             to_snake, current_snake);
        
        // Отмечаем змейку
        for (uint32_t id = 0; id < snake.Width(); ++id) {
            from_snake[from_begin + id] = to_snake[to_begin + id] = current_snake;
        }
        
        if (snake.Width() > 0) {
            ++current_snake;
        }
        
        // Обрабатываем правую часть
        GetSnakeDecomposition(from_end, from_right, to_end, to_right, from_snake,
                             to_snake, current_snake);
    } else if (from_right - from_left < to_right - to_left) {

        if (from_right == from_left) {
            return;
        }
        
        int32_t shift = 0;
        for (uint32_t id = 0; id < from_right - from_left; ++id) {
            if (from_tokens_[from_left + id] != to_tokens_[to_left + id + shift]) {
                ++current_snake;
                ++shift;
            }
            from_snake[from_left + id] = to_snake[to_left + id + shift] = current_snake;
        }
        ++current_snake;
    } else {

        if (to_right == to_left) {
            return;
        }
        
        int32_t shift = 0;
        for (uint32_t id = 0; id < to_right - to_left; ++id) {
            if (id + shift < from_right - from_left && 
                from_tokens_[from_left + id + shift] != to_tokens_[to_left + id]) {
                ++current_snake;
                ++shift;
            }
            if (id + shift < from_right - from_left) {
                from_snake[from_left + id + shift] = to_snake[to_left + id] = current_snake;
            }
        }
        ++current_snake;
    }
}

std::vector<TokenId> MyersDiff::GetLargestCommonSubsequence() const {
    if (from_tokens_.empty() || to_tokens_.empty()) {
        return {};
    }
    
    std::vector<int32_t> from_snake(from_tokens_.size(), -1);
    std::vector<int32_t> to_snake(to_tokens_.size(), -1);
    uint32_t current_snake = 0;
    
    GetSnakeDecomposition(0, from_tokens_.size(), 0, to_tokens_.size(), 
                         from_snake, to_snake, current_snake);
    
    std::vector<TokenId> lcs;
    for (uint32_t i = 0; i < from_tokens_.size(); ++i) {
        if (from_snake[i] != -1) {
            lcs.push_back(from_tokens_[i]);
        }
    }
    
    return lcs;
}

EditScript MyersDiff::GetShortestEditScript() const {
    if (from_tokens_.empty()) {
        if (!to_tokens_.empty()) {
            return {Replacement{0, 0, 0, static_cast<uint32_t>(to_tokens_.size())}};
        }
        return {};
    }
    
    if (to_tokens_.empty()) {
        return {Replacement{0, static_cast<uint32_t>(from_tokens_.size()), 0, 0}};
    }
    
    std::vector<int32_t> from_snake(from_tokens_.size(), -1);
    std::vector<int32_t> to_snake(to_tokens_.size(), -1);
    uint32_t current_snake = 0;
    
    GetSnakeDecomposition(0, from_tokens_.size(), 0, to_tokens_.size(), 
                         from_snake, to_snake, current_snake);
    
    EditScript script;
    uint32_t to_id = 0;
    
    for (uint32_t from_id = 0; from_id < from_tokens_.size(); ++from_id, ++to_id) {
        uint32_t from_left = from_id;
        
        while (from_id < from_tokens_.size() && from_snake[from_id] == -1) {
            ++from_id;
        }
        
        uint32_t to_left = to_id;
        
        while (to_id < to_tokens_.size() &&
               (from_id == from_tokens_.size() || to_snake[to_id] != from_snake[from_id])) {
            ++to_id;
        }
        
        if (from_left != from_id || to_left != to_id) {
            script.emplace_back(Replacement{from_left, from_id, to_left, to_id});
        }
        
        while (from_id + 1 < from_tokens_.size() && from_snake[from_id + 1] == from_snake[from_id]) {
            ++from_id;
        }
        
        while (to_id + 1 < to_tokens_.size() && to_snake[to_id + 1] == to_snake[to_id]) {
            ++to_id;
        }
    }
    
    if (to_id < to_tokens_.size()) {
        script.emplace_back(Replacement{static_cast<uint32_t>(from_tokens_.size()),
                                      static_cast<uint32_t>(from_tokens_.size()), to_id,
                                      static_cast<uint32_t>(to_tokens_.size())});
    }
    
    return script;
}

std::string MyersDiff::GetDiff(DiffFormat format, int context_size) const {
    EditScript script = GetShortestEditScript();
    
    switch (format) {
        case DiffFormat::UNIFIED:
            return FormatUnifiedDiff(script, context_size);
        case DiffFormat::CONTEXT:
            return FormatContextDiff(script, context_size);
        case DiffFormat::NORMAL:
            return FormatNormalDiff(script);
        default:
            return FormatUnifiedDiff(script, context_size);
    }
}

std::string MyersDiff::FormatUnifiedDiff(const EditScript& script, int context_size) const {
    if (script.empty()) {
        return ""; // Нет различий
    }
    
    std::stringstream result;
    result << "--- a" << std::endl;
    result << "+++ b" << std::endl;
    
    for (const auto& rep : script) {

        uint32_t from_start = (rep.from_left > static_cast<uint32_t>(context_size)) ? 
                             rep.from_left - context_size : 0;
        uint32_t from_end = std::min(rep.from_right + context_size, 
                                    static_cast<uint32_t>(from_tokens_.size()));
        uint32_t to_start = (rep.to_left > static_cast<uint32_t>(context_size)) ? 
                           rep.to_left - context_size : 0;
        uint32_t to_end = std::min(rep.to_right + context_size, 
                                  static_cast<uint32_t>(to_tokens_.size()));
        
        // Заголовок ханка
        result << "@@ -" << (from_start + 1) << "," << (from_end - from_start) 
               << " +" << (to_start + 1) << "," << (to_end - to_start) << " @@" << std::endl;
        
        // Выводим контекст до изменения
        for (uint32_t i = from_start; i < rep.from_left; ++i) {
            result << " " << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
        }
        
        // Выводим удаления
        for (uint32_t i = rep.from_left; i < rep.from_right; ++i) {
            result << "-" << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
        }
        
        // Выводим добавления
        for (uint32_t i = rep.to_left; i < rep.to_right; ++i) {
            result << "+" << tokenizer_->Decode({to_tokens_[i]}) << std::endl;
        }
        
        // Выводим контекст после изменения
        for (uint32_t i = rep.from_right; i < from_end; ++i) {
            result << " " << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
        }
    }
    
    return result.str();
}

std::string MyersDiff::FormatContextDiff(const EditScript& script, int context_size) const {
    if (script.empty()) {
        return ""; // Нет различий
    }
    
    std::stringstream result;
    result << "*** a" << std::endl;
    result << "--- b" << std::endl;
    
    for (const auto& rep : script) {
        // Вычисляем контекстные границы
        uint32_t from_start = (rep.from_left > static_cast<uint32_t>(context_size)) ? 
                             rep.from_left - context_size : 0;
        uint32_t from_end = std::min(rep.from_right + context_size, 
                                    static_cast<uint32_t>(from_tokens_.size()));
        uint32_t to_start = (rep.to_left > static_cast<uint32_t>(context_size)) ? 
                           rep.to_left - context_size : 0;
        uint32_t to_end = std::min(rep.to_right + context_size, 
                                  static_cast<uint32_t>(to_tokens_.size()));
        
        // Заголовок ханка для первого файла
        result << "***************" << std::endl;
        result << "*** " << (from_start + 1) << "," << (from_end) 
               << " ****" << std::endl;
        
        // Выводим контекст и удаления для первого файла
        for (uint32_t i = from_start; i < from_end; ++i) {
            if (i >= rep.from_left && i < rep.from_right) {
                result << "- " << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
            } else {
                result << "  " << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
            }
        }
        
        // Выводим заголовок ханка для второго файла
        result << "--- " << (to_start + 1) << "," << (to_end) 
               << " ----" << std::endl;
        
        // Выводим контекст и добавления для второго файла
        for (uint32_t i = to_start; i < to_end; ++i) {
            if (i >= rep.to_left && i < rep.to_right) {
                result << "+ " << tokenizer_->Decode({to_tokens_[i]}) << std::endl;
            } else {
                result << "  " << tokenizer_->Decode({to_tokens_[i]}) << std::endl;
            }
        }
    }
    
    return result.str();
}

std::string MyersDiff::FormatNormalDiff(const EditScript& script) const {
    if (script.empty()) {
        return ""; // Нет различий
    }
    
    std::stringstream result;
    
    for (const auto& rep : script) {
        if (rep.from_left == rep.from_right) {
            // Только добавления
            result << rep.from_left << "a" << rep.to_left + 1 
                   << "," << rep.to_right << std::endl;
            
            for (uint32_t i = rep.to_left; i < rep.to_right; ++i) {
                result << "> " << tokenizer_->Decode({to_tokens_[i]}) << std::endl;
            }
        } else if (rep.to_left == rep.to_right) {
            // Только удаления
            result << rep.from_left + 1 << "," << rep.from_right 
                   << "d" << rep.to_left << std::endl;
            
            for (uint32_t i = rep.from_left; i < rep.from_right; ++i) {
                result << "< " << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
            }
        } else {
            // Изменения
            result << rep.from_left + 1 << "," << rep.from_right 
                   << "c" << rep.to_left + 1 << "," << rep.to_right << std::endl;
            
            for (uint32_t i = rep.from_left; i < rep.from_right; ++i) {
                result << "< " << tokenizer_->Decode({from_tokens_[i]}) << std::endl;
            }
            
            result << "---" << std::endl;
            
            for (uint32_t i = rep.to_left; i < rep.to_right; ++i) {
                result << "> " << tokenizer_->Decode({to_tokens_[i]}) << std::endl;
            }
        }
    }
    
    return result.str();
}

int MyersDiff::GetLevenshteinDistance() const {
    EditScript script = GetShortestEditScript();
    int distance = 0;
    
    for (const auto& rep : script) {
        distance += (rep.from_right - rep.from_left) + (rep.to_right - rep.to_left);
    }
    
    return distance;
}

bool MyersDiff::AreTextsIdentical() const {

    if (from_tokens_.size() != to_tokens_.size()) {
        return false;
    }
    
    for (size_t i = 0; i < from_tokens_.size(); ++i) {
        if (from_tokens_[i] != to_tokens_[i]) {
            return false;
        }
    }
    
    return true;
}
