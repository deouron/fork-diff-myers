#include "UniversalTokenizer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <unordered_map>
#include <queue>
#include <utility>
#include <functional>
#include <limits>

UniversalTokenInfo::UniversalTokenInfo(TokenId id, const std::string& text)
    : id_(id), text_(text) {
}

UniversalTokenInfo::UniversalTokenInfo(TokenId id, std::string_view text)
    : id_(id), text_(text) {
}

bool UniversalTokenInfo::operator==(const UniversalTokenInfo& rhs) const {
    return id_ == rhs.id_;
}

TokenId UniversalTokenInfo::GetId() const {
    return id_;
}

std::string_view UniversalTokenInfo::GetText() const {
    return text_;
}

UniversalTokenizer::UniversalTokenizer(UniversalParserMode parser_mode)
    : parser_mode_(parser_mode) {
}

std::vector<std::string> UniversalTokenizer::SplitIntoUtf8Chars(const std::string& text) const {
    std::vector<std::string> chars;
    
    for (size_t i = 0; i < text.length(); ) {
        int char_len = 1;
        
        // Определяем длину UTF-8 символа
        if (parser_mode_ == UniversalParserMode::UTF_8) {
            if ((text[i] & 0xE0) == 0xC0) char_len = 2;
            else if ((text[i] & 0xF0) == 0xE0) char_len = 3;
            else if ((text[i] & 0xF8) == 0xF0) char_len = 4;
        }
        
        // Проверяем, не выходим ли за границы строки
        if (i + char_len <= text.length()) {
            chars.push_back(text.substr(i, char_len));
        } else {
            chars.push_back(text.substr(i, 1));
        }
        
        i += char_len;
    }
    
    return chars;
}

std::vector<std::string> UniversalTokenizer::SplitIntoWords(const std::string& text) const {
    std::vector<std::string> words;
    std::string current_word;
    
    auto chars = SplitIntoUtf8Chars(text);
    
    for (const auto& c : chars) {
        if (c == " " || c == "\t" || c == "\n" || c == "\r") {
            if (!current_word.empty()) {
                words.push_back(current_word);
                current_word.clear();
            }
            // Добавляем пробел как отдельный токен
            words.push_back(c);
        } else {
            current_word += c;
        }
    }
    
    if (!current_word.empty()) {
        words.push_back(current_word);
    }
    
    return words;
}

bool UniversalTokenizer::IsUtf8Char(char c) const {
    return parser_mode_ == UniversalParserMode::UTF_8 && (c & 0x80);
}

BPETokenizer::BPETokenizer(UniversalParserMode parser_mode)
    : UniversalTokenizer(parser_mode) {
    vocab_["<unk>"] = 0;
    vocab_["<s>"] = 1;
    vocab_["</s>"] = 2;
    vocab_["<pad>"] = 3;
    
    inverse_vocab_[0] = "<unk>";
    inverse_vocab_[1] = "<s>";
    inverse_vocab_[2] = "</s>";
    inverse_vocab_[3] = "<pad>";
}

std::vector<TokenId> BPETokenizer::Encode(const std::string& text) const {
    std::vector<TokenId> result;
    
    auto words = SplitIntoWords(text);
    
    for (const auto& word : words) {
        auto word_tokens = ApplyBPE(word);
        
        for (const auto& token : word_tokens) {
            auto it = vocab_.find(token);
            if (it != vocab_.end()) {
                result.push_back(it->second);
            } else {
                result.push_back(vocab_.at("<unk>"));
            }
        }
    }
    
    return result;
}

std::string BPETokenizer::Decode(const std::vector<TokenId>& tokens) const {
    std::string result;
    
    for (auto token_id : tokens) {
        auto it = inverse_vocab_.find(token_id);
        if (it != inverse_vocab_.end()) {
            result += it->second;
        } else {
            result += inverse_vocab_.at(0);
        }
    }
    
    return result;
}

const std::unordered_map<std::string, TokenId>& BPETokenizer::GetVocabulary() const {
    return vocab_;
}

bool BPETokenizer::SaveVocabulary(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file) {
        return false;
    }
    
    for (const auto& [token, id] : vocab_) {
        file << token << "\t" << id << "\n";
    }
    
    file << "# Merges\n";
    for (const auto& [first, second] : merges_) {
        file << first << " " << second << "\n";
    }
    
    return true;
}

bool BPETokenizer::LoadVocabulary(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file) {
        return false;
    }
    
    vocab_.clear();
    inverse_vocab_.clear();
    merges_.clear();
    
    std::string line;
    bool in_merges_section = false;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            if (line == "# Merges") {
                in_merges_section = true;
            }
            continue;
        }
        
        if (in_merges_section) {
            std::istringstream iss(line);
            std::string first, second;
            if (iss >> first >> second) {
                merges_.emplace_back(first, second);
            }
        } else {
            std::istringstream iss(line);
            std::string token;
            TokenId id;
            if (std::getline(iss, token, '\t') && iss >> id) {
                vocab_[token] = id;
                inverse_vocab_[id] = token;
            }
        }
    }
    
    return true;
}

void BPETokenizer::Train(const std::vector<std::string>& corpus, int vocab_size, int min_frequency) {

    vocab_.clear();
    inverse_vocab_.clear();
    merges_.clear();
    
    vocab_["<unk>"] = 0;
    vocab_["<s>"] = 1;
    vocab_["</s>"] = 2;
    vocab_["<pad>"] = 3;
    
    inverse_vocab_[0] = "<unk>";
    inverse_vocab_[1] = "<s>";
    inverse_vocab_[2] = "</s>";
    inverse_vocab_[3] = "<pad>";
    
    TokenId next_id = 4; 
    
    std::unordered_map<std::string, int> char_counts;
    std::vector<std::vector<std::string>> tokenized_corpus;
    
    for (const auto& text : corpus) {
        std::vector<std::string> chars;
        for (const auto& word : SplitIntoWords(text)) {
            auto word_chars = SplitIntoUtf8Chars(word);
            for (const auto& c : word_chars) {
                char_counts[c]++;
                chars.push_back(c);
            }
        }
        tokenized_corpus.push_back(chars);
    }
    
    for (const auto& [c, count] : char_counts) {
        if (count >= min_frequency && vocab_.find(c) == vocab_.end()) {
            vocab_[c] = next_id;
            inverse_vocab_[next_id] = c;
            next_id++;
        }
    }
    
    while (vocab_.size() < static_cast<size_t>(vocab_size)) {
        std::unordered_map<std::string, int> pair_counts;
        
        for (auto& tokens : tokenized_corpus) {
            for (size_t i = 0; i < tokens.size() - 1; i++) {
                std::string pair = tokens[i] + " " + tokens[i + 1];
                pair_counts[pair]++;
            }
        }

        std::string best_pair;
        int best_count = 0;
        
        for (const auto& [pair, count] : pair_counts) {
            if (count > best_count) {
                best_count = count;
                best_pair = pair;
            }
        }
        
        if (best_count < min_frequency || best_pair.empty()) {
            break;
        }
        
        std::istringstream iss(best_pair);
        std::string first, second;
        iss >> first >> second;
        
        std::string new_token = first + second;
        if (vocab_.find(new_token) == vocab_.end()) {
            vocab_[new_token] = next_id;
            inverse_vocab_[next_id] = new_token;
            next_id++;
            
            merges_.emplace_back(first, second);
        }
        
        for (auto& tokens : tokenized_corpus) {
            for (size_t i = 0; i < tokens.size() - 1; i++) {
                if (tokens[i] == first && tokens[i + 1] == second) {
                    tokens[i] = new_token;
                    tokens.erase(tokens.begin() + i + 1);
                    i--;
                }
            }
        }
    }
}

void BPETokenizer::AddMerges(const std::vector<std::pair<std::string, std::string>>& merges) {
    for (const auto& merge : merges) {
        // Добавляем составляющие в словарь, если их еще нет
        if (vocab_.find(merge.first) == vocab_.end()) {
            TokenId new_id = vocab_.size();
            vocab_[merge.first] = new_id;
            inverse_vocab_[new_id] = merge.first;
        }
        
        if (vocab_.find(merge.second) == vocab_.end()) {
            TokenId new_id = vocab_.size();
            vocab_[merge.second] = new_id;
            inverse_vocab_[new_id] = merge.second;
        }
        
        // Добавляем объединенный токен в словарь
        std::string new_token = merge.first + merge.second;
        if (vocab_.find(new_token) == vocab_.end()) {
            TokenId new_id = vocab_.size();
            vocab_[new_token] = new_id;
            inverse_vocab_[new_id] = new_token;
        }
        
        merges_.push_back(merge);
    }
}


std::vector<std::string> BPETokenizer::ApplyBPE(const std::string& word) const {
    if (word.empty()) {
        return {};
    }
    
    std::vector<std::string> tokens = SplitIntoUtf8Chars(word);
    
    for (const auto& [first, second] : merges_) {
        for (size_t i = 0; i < tokens.size() - 1; ++i) {
            if (tokens[i] == first && tokens[i + 1] == second) {
                tokens[i] = first + second;
                tokens.erase(tokens.begin() + i + 1);
                --i; 
            }
        }
    }
    
    return tokens;
}

CharacterTokenizer::CharacterTokenizer(UniversalParserMode parser_mode)
    : UniversalTokenizer(parser_mode) {

    vocab_["<unk>"] = 0;
    vocab_[" "] = 1;  
    vocab_["\t"] = 2; 
    vocab_["\n"] = 3; 
    
    inverse_vocab_[0] = "<unk>";
    inverse_vocab_[1] = " ";
    inverse_vocab_[2] = "\t";
    inverse_vocab_[3] = "\n";
}

std::vector<TokenId> CharacterTokenizer::Encode(const std::string& text) const {
    std::vector<TokenId> result;
    auto chars = SplitIntoUtf8Chars(text);
    
    for (const auto& c : chars) {
        auto it = vocab_.find(c);
        if (it != vocab_.end()) {
            result.push_back(it->second);
        } else {
            TokenId new_id = vocab_.size();
            const_cast<CharacterTokenizer*>(this)->vocab_[c] = new_id;
            const_cast<CharacterTokenizer*>(this)->inverse_vocab_[new_id] = c;
            result.push_back(new_id);
        }
    }
    
    return result;
}

std::string CharacterTokenizer::Decode(const std::vector<TokenId>& tokens) const {
    std::string result;
    
    for (auto token_id : tokens) {
        auto it = inverse_vocab_.find(token_id);
        if (it != inverse_vocab_.end()) {
            result += it->second;
        } else {
            result += inverse_vocab_.at(0); // <unk>
        }
    }
    
    return result;
}

const std::unordered_map<std::string, TokenId>& CharacterTokenizer::GetVocabulary() const {
    return vocab_;
}

bool CharacterTokenizer::SaveVocabulary(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file) {
        return false;
    }
    
    for (const auto& [token, id] : vocab_) {
        file << token << "\t" << id << "\n";
    }
    
    return true;
}

bool CharacterTokenizer::LoadVocabulary(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file) {
        return false;
    }
    
    vocab_.clear();
    inverse_vocab_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        TokenId id;
        
        if (std::getline(iss, token, '\t') && iss >> id) {
            vocab_[token] = id;
            inverse_vocab_[id] = token;
        }
    }
    
    return true;
}

// Реализация WordTokenizer
WordTokenizer::WordTokenizer(UniversalParserMode parser_mode)
    : UniversalTokenizer(parser_mode) {

    vocab_["<unk>"] = 0;
    vocab_[" "] = 1;
    vocab_["\t"] = 2;
    vocab_["\n"] = 3;
    
    inverse_vocab_[0] = "<unk>";
    inverse_vocab_[1] = " ";
    inverse_vocab_[2] = "\t";
    inverse_vocab_[3] = "\n";
}

std::vector<TokenId> WordTokenizer::Encode(const std::string& text) const {
    std::vector<TokenId> result;
    auto words = SplitIntoWords(text);
    
    for (const auto& word : words) {
        auto it = vocab_.find(word);
        if (it != vocab_.end()) {
            result.push_back(it->second);
        } else {
            TokenId new_id = vocab_.size();
            const_cast<WordTokenizer*>(this)->vocab_[word] = new_id;
            const_cast<WordTokenizer*>(this)->inverse_vocab_[new_id] = word;
            result.push_back(new_id);
        }
    }
    
    return result;
}

std::string WordTokenizer::Decode(const std::vector<TokenId>& tokens) const {
    std::string result;
    
    for (auto token_id : tokens) {
        auto it = inverse_vocab_.find(token_id);
        if (it != inverse_vocab_.end()) {
            result += it->second;
        } else {
            result += inverse_vocab_.at(0); // <unk>
        }
    }
    
    return result;
}

const std::unordered_map<std::string, TokenId>& WordTokenizer::GetVocabulary() const {
    return vocab_;
}

bool WordTokenizer::SaveVocabulary(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file) {
        return false;
    }
    
    for (const auto& [token, id] : vocab_) {
        file << token << "\t" << id << "\n";
    }
    
    return true;
}

bool WordTokenizer::LoadVocabulary(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file) {
        return false;
    }
    
    vocab_.clear();
    inverse_vocab_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        TokenId id;
        
        if (std::getline(iss, token, '\t') && iss >> id) {
            vocab_[token] = id;
            inverse_vocab_[id] = token;
        }
    }
    
    return true;
}

WhitespaceTokenizer::WhitespaceTokenizer(UniversalParserMode parser_mode)
    : UniversalTokenizer(parser_mode) {

    vocab_["<unk>"] = 0;
    inverse_vocab_[0] = "<unk>";
}

std::vector<TokenId> WhitespaceTokenizer::Encode(const std::string& text) const {
    std::vector<TokenId> result;
    
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token) {
        auto it = vocab_.find(token);
        if (it != vocab_.end()) {
            result.push_back(it->second);
        } else {
            TokenId new_id = vocab_.size();
            const_cast<WhitespaceTokenizer*>(this)->vocab_[token] = new_id;
            const_cast<WhitespaceTokenizer*>(this)->inverse_vocab_[new_id] = token;
            result.push_back(new_id);
        }
    }
    
    return result;
}

std::string WhitespaceTokenizer::Decode(const std::vector<TokenId>& tokens) const {
    std::string result;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        auto token_id = tokens[i];
        auto it = inverse_vocab_.find(token_id);
        
        if (it != inverse_vocab_.end()) {
            result += it->second;
            if (i < tokens.size() - 1) {
                result += " "; 
            }
        } else {
            result += inverse_vocab_.at(0); // <unk>
            if (i < tokens.size() - 1) {
                result += " ";
            }
        }
    }
    
    return result;
}

const std::unordered_map<std::string, TokenId>& WhitespaceTokenizer::GetVocabulary() const {
    return vocab_;
}

bool WhitespaceTokenizer::SaveVocabulary(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file) {
        return false;
    }
    
    for (const auto& [token, id] : vocab_) {
        file << token << "\t" << id << "\n";
    }
    
    return true;
}

bool WhitespaceTokenizer::LoadVocabulary(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file) {
        return false;
    }
    
    vocab_.clear();
    inverse_vocab_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        TokenId id;
        
        if (std::getline(iss, token, '\t') && iss >> id) {
            vocab_[token] = id;
            inverse_vocab_[id] = token;
        }
    }
    
    return true;
}

std::unique_ptr<UniversalTokenizer> CreateTokenizer(
    UniversalTokenizerMode mode,
    UniversalParserMode parser_mode
) {
    switch (mode) {
        case UniversalTokenizerMode::BPE:
            return std::make_unique<BPETokenizer>(parser_mode);
        case UniversalTokenizerMode::CHARACTER:
            return std::make_unique<CharacterTokenizer>(parser_mode);
        case UniversalTokenizerMode::WORD:
            return std::make_unique<WordTokenizer>(parser_mode);
        case UniversalTokenizerMode::WHITESPACE:
            return std::make_unique<WhitespaceTokenizer>(parser_mode);
        default:
            throw std::invalid_argument("Unknown tokenizer mode");
    }
}
