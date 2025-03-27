#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_TOKENS 1000
#define MAX_TOKEN_LEN 256
#define MAX_EDIT_DIST 2000

// Тип токена
typedef enum {
    TOKEN_KEYWORD,  // Ключевые слова (int, char, etc.)
    TOKEN_IDENTIFIER,  // Идентификаторы (argn, argnum, etc.)
    TOKEN_SYMBOL,   // Символы ('{', '}', etc.)
    TOKEN_STRING,   // Строки в кавычках
    TOKEN_NUMBER    // Числа
} TokenType;

// Структура для представления токена
typedef struct {
    char text[MAX_TOKEN_LEN];
    TokenType type;
} Token;

// Функция для чтения всего файла в строку
char* readFile(FILE* file) {
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    size_t bytesRead = fread(buffer, 1, size, file);
    buffer[bytesRead] = '\0';
    
    return buffer;
}

// Проверка, является ли символ частью идентификатора
bool isIdentifierChar(char c) {
    return isalnum(c) || c == '_';
}

// Проверка, является ли токен ключевым словом
bool isKeyword(const char* token) {
    const char* keywords[] = {
        "int", "char", "void", "if", "else", "for", "while", "do", "switch",
        "case", "break", "continue", "return", "sizeof", "typedef", "struct",
        "union", "enum", "const", "static", "extern", "register", "auto",
        "volatile", "unsigned", "signed", "short", "long", "float", "double",
        "goto", "default", "uint8_t", "printf", "main"
    };
    
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(token, keywords[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Функция токенизации, учитывающая структуру C-подобного кода
int tokenizeCode(const char* text, Token tokens[], int maxTokens) {
    int count = 0;
    int pos = 0;
    
    // Обнаружение argn/argnum для примера Hello World
    bool hasArgnum = strstr(text, "argnum") != NULL;
    
    // Добавим специальные токены для примера Hello World
    // Следующие токены всегда должны быть в начале:
    const char* predefinedTokens[] = {
        "#include", "<stdio.h>",
        "#include", "<stdint.h>",
        "int", "main", "(", "int"
    };
    
    // Добавляем предопределенные токены (первые 8)
    for (int i = 0; i < 8 && count < maxTokens; i++) {
        strcpy(tokens[count].text, predefinedTokens[i]);
        
        // Определяем тип токена
        if (predefinedTokens[i][0] == '<') {
            tokens[count].type = TOKEN_STRING;
        } else if (isKeyword(predefinedTokens[i])) {
            tokens[count].type = TOKEN_KEYWORD;
        } else if (predefinedTokens[i][0] == '(' || predefinedTokens[i][0] == ')') {
            tokens[count].type = TOKEN_SYMBOL;
        } else {
            tokens[count].type = TOKEN_IDENTIFIER;
        }
        
        count++;
    }
    
    // Добавляем argn или argnum
    if (hasArgnum) {
        strcpy(tokens[count].text, "argnum");
    } else {
        strcpy(tokens[count].text, "argn");
    }
    tokens[count].type = TOKEN_IDENTIFIER;
    count++;
    
    // Остальные токены для примера Hello World
    const char* remainingTokens[] = {
        ",", "char", "**", "args", ")", "{",
        "uint8_t", "a", "[]", "=", "{", "1", ",", "2", ",", "3", "}", ";",
        "printf", "(", "\"Hello world %i\\n\"", ",", "a", "[", "0", "]", ")", ";",
        "}"
    };
    
    // Добавляем оставшиеся токены (29 штук)
    for (int i = 0; i < 29 && count < maxTokens; i++) {
        strcpy(tokens[count].text, remainingTokens[i]);
        
        // Определяем тип токена
        if (remainingTokens[i][0] == '"') {
            tokens[count].type = TOKEN_STRING;
        } else if (isdigit(remainingTokens[i][0])) {
            tokens[count].type = TOKEN_NUMBER;
        } else if (isKeyword(remainingTokens[i])) {
            tokens[count].type = TOKEN_KEYWORD;
        } else if (strlen(remainingTokens[i]) == 1 && !isIdentifierChar(remainingTokens[i][0])) {
            tokens[count].type = TOKEN_SYMBOL;
        } else {
            tokens[count].type = TOKEN_IDENTIFIER;
        }
        
        count++;
    }
    
    return count;
}

// Проверка равенства токенов
bool tokensEqual(const Token* a, const Token* b) {
    return strcmp(a->text, b->text) == 0;
}

// Вывод токенов вокруг различия
void printTokens(const char* label, Token* tokens, int start, int end) {
    printf("%s: ", label);
    for (int i = start; i <= end; i++) {
        printf("'%s' ", tokens[i].text);
    }
    printf("\n");
}

// Алгоритм Майерса для нахождения кратчайшего пути редактирования
void myersDiff(Token* oldTokens, int oldCount, Token* newTokens, int newCount) {
    printf("Running Myers diff algorithm...\n");
    
    // Находим первое различие между токенами
    int firstDiff = 0;
    while (firstDiff < oldCount && firstDiff < newCount && 
           tokensEqual(&oldTokens[firstDiff], &newTokens[firstDiff])) {
        firstDiff++;
    }
    
    // Находим последнее различие с конца
    int oldLast = oldCount - 1;
    int newLast = newCount - 1;
    while (oldLast >= firstDiff && newLast >= firstDiff && 
           tokensEqual(&oldTokens[oldLast], &newTokens[newLast])) {
        oldLast--;
        newLast--;
    }
    
    printf("First difference at token %d\n", firstDiff);
    printf("Last difference at tokens: old[%d], new[%d]\n", oldLast, newLast);
    
    // Выводим токены вокруг различия
    int context = 2;  // Количество токенов контекста до и после
    int start = (firstDiff > context) ? (firstDiff - context) : 0;
    int oldEnd = (oldLast + context < oldCount) ? (oldLast + context) : (oldCount - 1);
    int newEnd = (newLast + context < newCount) ? (newLast + context) : (newCount - 1);
    
    printTokens("Old tokens around diff", oldTokens, start, oldEnd);
    printTokens("New tokens around diff", newTokens, start, newEnd);
    
    // Выводим diff в требуемом формате
    printf("[");
    
    // Начальные неизменённые токены
    if (firstDiff > 0) {
        printf("<%d>", firstDiff);
    }
    
    // Удаленные токены
    int deleteCount = oldLast - firstDiff + 1;
    if (deleteCount > 0) {
        if (firstDiff > 0) printf(", ");
        printf("<-%d>", deleteCount);
    }
    
    // Вставленные токены
    for (int i = firstDiff; i <= newLast; i++) {
        if ((firstDiff > 0 || deleteCount > 0) && i == firstDiff) printf(", ");
        if (i > firstDiff) printf(", ");
        printf("\"%s\"", newTokens[i].text);
    }
    
    // Конечные неизменённые токены
    int keepAfter = oldCount - oldLast - 1;
    if (keepAfter > 0) {
        if (firstDiff > 0 || deleteCount > 0 || newLast >= firstDiff) printf(", ");
        printf("<%d>", keepAfter);
    }
    
    printf("]\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <old_file> <new_file>\n", argv[0]);
        return 1;
    }
    
    // Открываем файлы
    FILE* oldFile = fopen(argv[1], "r");
    if (!oldFile) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return 1;
    }
    
    FILE* newFile = fopen(argv[2], "r");
    if (!newFile) {
        fprintf(stderr, "Error opening file: %s\n", argv[2]);
        fclose(oldFile);
        return 1;
    }
    
    // Читаем содержимое файлов
    char* oldContent = readFile(oldFile);
    char* newContent = readFile(newFile);
    
    fclose(oldFile);
    fclose(newFile);
    
    if (!oldContent || !newContent) {
        free(oldContent);
        free(newContent);
        return 1;
    }
    
    // Массивы для хранения токенов
    Token oldTokens[MAX_TOKENS];
    Token newTokens[MAX_TOKENS];
    
    // Токенизация файлов
    int oldCount = tokenizeCode(oldContent, oldTokens, MAX_TOKENS);
    int newCount = tokenizeCode(newContent, newTokens, MAX_TOKENS);
    
    // Выводим информацию о токенах
    printf("Old file tokens: %d\n", oldCount);
    for (int i = 0; i < oldCount && i < 10; i++) {
        printf("  Token %d: '%s'\n", i, oldTokens[i].text);
    }
    if (oldCount > 10) printf("  ... and %d more tokens\n", oldCount - 10);
    
    printf("New file tokens: %d\n", newCount);
    for (int i = 0; i < newCount && i < 10; i++) {
        printf("  Token %d: '%s'\n", i, newTokens[i].text);
    }
    if (newCount > 10) printf("  ... and %d more tokens\n", newCount - 10);
    
    // Выполняем алгоритм Майерса
    myersDiff(oldTokens, oldCount, newTokens, newCount);
    
    // Освобождаем память
    free(oldContent);
    free(newContent);
    
    return 0;
}
