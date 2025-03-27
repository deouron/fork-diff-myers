#pragma once

#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <unordered_set>
#include <string_view>
#include <functional>

using TokenId = uint32_t;

enum class UniversalParserMode { BYTES, UTF_8 };

enum class UniversalTokenizerMode {
    BPE,           // Байт-паир кодирование
    WORD,          // По словам
    CHARACTER,     // По символам
    WHITESPACE     // По пробелам
};

class UniversalTokenInfo {
public:
    UniversalTokenInfo(TokenId id, const std::string& text);
    UniversalTokenInfo(TokenId id, std::string_view text);

    bool operator==(const UniversalTokenInfo& rhs) const;

    TokenId GetId() const;
    std::string_view GetText() const;

private:
    TokenId id_;
    std::string text_;
};

class UniversalTokenizer {
public:
    UniversalTokenizer(UniversalParserMode parser_mode);
    virtual ~UniversalTokenizer() = default;

    virtual std::vector<TokenId> Encode(const std::string& text) const = 0;
    virtual std::string Decode(const std::vector<TokenId>& tokens) const = 0;

    virtual const std::unordered_map<std::string, TokenId>& GetVocabulary() const = 0;
    
    virtual bool SaveVocabulary(const std::string& file_path) const = 0;
    virtual bool LoadVocabulary(const std::string& file_path) = 0;

protected:
    std::vector<std::string> SplitIntoUtf8Chars(const std::string& text) const;
    std::vector<std::string> SplitIntoWords(const std::string& text) const;
    
    bool IsUtf8Char(char c) const;
    
    UniversalParserMode parser_mode_;
};

class BPETokenizer : public UniversalTokenizer {
public:
    BPETokenizer(UniversalParserMode parser_mode);
    
    std::vector<TokenId> Encode(const std::string& text) const override;
    std::string Decode(const std::vector<TokenId>& tokens) const override;
    
    const std::unordered_map<std::string, TokenId>& GetVocabulary() const override;
    
    bool SaveVocabulary(const std::string& file_path) const override;
    bool LoadVocabulary(const std::string& file_path) override;
    
    void Train(const std::vector<std::string>& corpus, int vocab_size, int min_frequency = 2);
    void AddMerges(const std::vector<std::pair<std::string, std::string>>& merges);
    
private:
    std::unordered_map<std::string, TokenId> vocab_;
    std::unordered_map<TokenId, std::string> inverse_vocab_;
    
    std::vector<std::pair<std::string, std::string>> merges_;
    
    std::vector<std::string> ApplyBPE(const std::string& word) const;
};

class CharacterTokenizer : public UniversalTokenizer {
public:
    CharacterTokenizer(UniversalParserMode parser_mode);
    
    std::vector<TokenId> Encode(const std::string& text) const override;
    std::string Decode(const std::vector<TokenId>& tokens) const override;
    
    const std::unordered_map<std::string, TokenId>& GetVocabulary() const override;
    
    bool SaveVocabulary(const std::string& file_path) const override;
    bool LoadVocabulary(const std::string& file_path) override;
    
private:
    std::unordered_map<std::string, TokenId> vocab_;
    std::unordered_map<TokenId, std::string> inverse_vocab_;
};

class WordTokenizer : public UniversalTokenizer {
public:
    WordTokenizer(UniversalParserMode parser_mode);
    
    std::vector<TokenId> Encode(const std::string& text) const override;
    std::string Decode(const std::vector<TokenId>& tokens) const override;
    
    const std::unordered_map<std::string, TokenId>& GetVocabulary() const override;
    
    bool SaveVocabulary(const std::string& file_path) const override;
    bool LoadVocabulary(const std::string& file_path) override;
    
private:
    std::unordered_map<std::string, TokenId> vocab_;
    std::unordered_map<TokenId, std::string> inverse_vocab_;
};

class WhitespaceTokenizer : public UniversalTokenizer {
public:
    WhitespaceTokenizer(UniversalParserMode parser_mode);
    
    std::vector<TokenId> Encode(const std::string& text) const override;
    std::string Decode(const std::vector<TokenId>& tokens) const override;
    
    const std::unordered_map<std::string, TokenId>& GetVocabulary() const override;
    
    bool SaveVocabulary(const std::string& file_path) const override;
    bool LoadVocabulary(const std::string& file_path) override;
    
private:
    std::unordered_map<std::string, TokenId> vocab_;
    std::unordered_map<TokenId, std::string> inverse_vocab_;
};

std::unique_ptr<UniversalTokenizer> CreateTokenizer(
    UniversalTokenizerMode mode,
    UniversalParserMode parser_mode = UniversalParserMode::UTF_8
);
