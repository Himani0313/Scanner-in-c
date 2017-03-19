#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define BACKWARD_SLASH '\\'
#define FORWARD_SLASH '/'
#define DOUBLE_QUOTE '"'
#define SINGLE_QUOTE '\''
#define NEW_LINE '\n'
#define HASH '#'
#define ASTERISK '*'

char* arithmeticOperators[] = { "++", "--", "+=", "-=", "*=", "/=", "%=", "=", "+", "-", "*", "/", "%" };

char* relationalOperators[] = { "==", "!=", ">=", "<=", ">", "<" };

char* logicalOperators[] = { "!", "&&", "||" };

char* memberPointerOperators[] = { "*", "&", "->", "." };

char* bitwiseOperators[] = { "^=", "<<=", ">>=", "^", "<<", ">>", "&=", "|=", "~", "&", "|" };

char* keywords[] = { "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while" };

enum { FALSE, TRUE };

typedef enum Type { INIT, PREPROCESSOR, KEYWORD, IDENTIFIER, POINTER, DELIMITER, ARITHMETIC_OPERATOR, RELATIONAL_OPERATOR, LOGICAL_OPERATOR, MEMBER_POINTER_OPERATOR, BITWISE_OPERATOR, STRING, NUMBER, CHARACTER, SINGLE_LINE_COMMENT, MULTI_LINE_COMMENT, WHITE_SPACE } Type;

char* getTypeName(Type type) {
   switch (type) {
        case INIT: return "INIT";
        case PREPROCESSOR: return "PREPROCESSOR";
        case KEYWORD: return "KEYWORD";
        case IDENTIFIER: return "IDENTIFIER";
        case POINTER: return "POINTER";
        case DELIMITER: return "DELIMITER";
        case ARITHMETIC_OPERATOR: return "ARITHMETIC_OPERATOR";
        case RELATIONAL_OPERATOR: return "RELATIONAL_OPERATOR";
        case LOGICAL_OPERATOR: return "LOGICAL_OPERATOR";
        case MEMBER_POINTER_OPERATOR: return "MEMBER_POINTER_OPERATOR";
        case BITWISE_OPERATOR: return "BITWISE_OPERATOR";
        case STRING: return "STRING";
        case NUMBER: return "NUMBER";
        case CHARACTER: return "CHARACTER";
        case SINGLE_LINE_COMMENT: return "SINGLE_LINE_COMMENT";
        case MULTI_LINE_COMMENT: return "MULTI_LINE_COMMENT";
        case WHITE_SPACE: return "WHITE_SPACE";
   }
}

typedef struct Scanner {
    FILE* sfile;
    FILE* dfile;

    char previous;
    char current;
    char next;

    int line;
    int characters;

    Type previousType;
} Scanner;

void checkFile(FILE* file) {
    if(file == NULL) {
        perror("ERROR OPENING FILE");
        exit(1);
    }
}

FILE* openFile(const char* name, const char* mode) {
    FILE* file;
    file = fopen(name, mode);
    checkFile(file);
    return file;
}

void closeFile(FILE* file) {
    if(fclose(file) == EOF) {
        perror("ERROR CLOSING FILE");
        exit(1);
    }
}

void initScanner(Scanner* scanner, char* sourceFileLocation, char* destinationFileLocation) {
    scanner->sfile = openFile(sourceFileLocation, "r");
    scanner->dfile = openFile(destinationFileLocation, "w");

    scanner->previous = EOF;
    scanner->current = EOF;
    scanner->next = EOF;

    scanner->line = 1;
    scanner->characters = 0;

    scanner->previousType = INIT;
}

void closeScanner(Scanner* scanner) {
    closeFile(scanner->sfile);
    closeFile(scanner->dfile);
}

int getChar(Scanner* scanner) {
    int temp = fgetc(scanner->sfile);

    if(temp != EOF) {
        scanner->characters++;
    }

    return temp;
}

void restoreChar(Scanner* scanner, char backupChar) {
    scanner->characters--;

    ungetc(scanner->next, scanner->sfile);

    scanner->next = scanner->current;
    scanner->current = scanner->previous;
    scanner->previous = backupChar;
}

int readAndSetCurrentChar(Scanner* scanner) {
    return scanner->current = getChar(scanner);
}

int readAndSetNextChar(Scanner* scanner) {
    return scanner->next = getChar(scanner);
}

int isCurrentEOF(Scanner* scanner) {
    return scanner->current == EOF;
}

void swapChars(Scanner* scanner) {
    scanner->previous = scanner->current;
    scanner->current = scanner->next;
}

void reportPreviousType(Scanner* scanner) {
    fprintf(scanner->dfile, "\t<%s>\n", getTypeName(scanner->previousType));
}

void setAndReportPreviousType(Scanner* scanner, Type type) {
    scanner->previousType = type;
    reportPreviousType(scanner);
}

int isSingleLineComment(char current, char next) {
    return current == FORWARD_SLASH && next == FORWARD_SLASH;
}

int isDoubleQuote(char previous, char current) {
    return previous != BACKWARD_SLASH && current == DOUBLE_QUOTE;
}

int isSingleQuote(char previous, char current) {
    return previous != BACKWARD_SLASH && current == SINGLE_QUOTE;
}

int isNewLine(char current) {
    return current == NEW_LINE;
}

int isMultiLineComment(char current, char next) {
    return current == FORWARD_SLASH && next == ASTERISK;
}

int isHash(char current) {
    return current == '#';
}

int isDelimiter(char current) {
    return current == ',' || current == '.' || current == ':' || current == ';' || current == '?' || current == '(' || current == ')' || current == '{' || current == '}' || current == '[' || current == ']';
}

void reportNewLine(Scanner* scanner) {
    fprintf(scanner->dfile, "\n");
}

void startNewLine(Scanner* scanner) {
    fprintf(scanner->dfile, "----------Line %d", scanner->line);
    reportNewLine(scanner);
}

void incrementAndReportNewLine(Scanner* scanner) {
    scanner->line++;
    reportNewLine(scanner);
    startNewLine(scanner);
}

void reportChar(Scanner* scanner, char c) {
    fprintf(scanner->dfile, "%c", c);
}

void reportCurrentChar(Scanner* scanner) {
    reportChar(scanner, scanner->current);
}

int isCurrentNotEOFOrReadAndSetCurrentChar(Scanner* scanner) {
    return ! isCurrentEOF(scanner) || readAndSetCurrentChar(scanner) != EOF;
}

char readPreProcessor(Scanner* scanner) {
    char backupChar;

    reportCurrentChar(scanner);
    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);

        if(scanner->current == NEW_LINE) break;

        backupChar = scanner->previous;

        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));

    setAndReportPreviousType(scanner, PREPROCESSOR);

    return backupChar;
}

void stripMultiLineComment(Scanner* scanner) {
    reportCurrentChar(scanner);
    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);

        if(scanner->current == NEW_LINE) {
            incrementAndReportNewLine(scanner);
        } else if(scanner->previous == ASTERISK && scanner->current == FORWARD_SLASH) {
            break;
        }

        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));
    reportCurrentChar(scanner);
    setAndReportPreviousType(scanner, MULTI_LINE_COMMENT);
}

void stripSingleLineComment(Scanner* scanner) {
    reportCurrentChar(scanner);
    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);

        if(scanner->current == NEW_LINE) {
            scanner->line++;
            break;
        }

        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));

    setAndReportPreviousType(scanner, SINGLE_LINE_COMMENT);
}

void readString(Scanner* scanner) {
    int backwardSlashCount = 0;

    reportCurrentChar(scanner);
    setAndReportPreviousType(scanner, DELIMITER);

    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);
        
        if(scanner->previous == BACKWARD_SLASH) backwardSlashCount++;
        else backwardSlashCount = 0;

        if(scanner->current == DOUBLE_QUOTE && (backwardSlashCount == 0 || backwardSlashCount % 2 == 0)) break;

        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));

    setAndReportPreviousType(scanner, STRING);

    reportCurrentChar(scanner);
    setAndReportPreviousType(scanner, DELIMITER);
}

void readChar(Scanner* scanner) {
    int backwardSlashCount = 0;

    reportCurrentChar(scanner);
    setAndReportPreviousType(scanner, DELIMITER);

    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);

        if(scanner->previous == BACKWARD_SLASH) backwardSlashCount++;
        else backwardSlashCount = 0;

        if(scanner->current == SINGLE_QUOTE && (backwardSlashCount == 0 || backwardSlashCount % 2 == 0)) break;

        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));

    setAndReportPreviousType(scanner, CHARACTER);

    reportCurrentChar(scanner);
    setAndReportPreviousType(scanner, DELIMITER);
}

char readKeywordIdentifier(Scanner* scanner) {
    char keyword[10];
    int index = 0;
    int i = 0;
    int isKeywork = 0;
    char backupChar;

    reportCurrentChar(scanner);
    keyword[index++] = scanner->current;
    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);

        if(isalpha(scanner->current) && index <= 8) {
            keyword[index++] = scanner->current;
        }

        if( ! (isalnum(scanner->current) || scanner->current == '_')) {
            break;
        }

        backupChar = scanner->previous;

        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));

    keyword[index] = '\0';

    if(index < 9) {
        for(i = 0; i < 32; i++) {
            if(strcmp(keywords[i], keyword) == 0) {
                scanner->previousType = KEYWORD;
                isKeywork = 1;

                break;
            }
        }
    }

    if(isKeywork == 0) scanner->previousType = IDENTIFIER;

    reportPreviousType(scanner);

    return backupChar;
}

char readDigit(Scanner* scanner) {
    char backupChar;

    reportCurrentChar(scanner);
    do {
        swapChars(scanner);
        readAndSetNextChar(scanner);

        if( ! (isalnum(scanner->current) || scanner->current == '_')) {
            break;
        }

        backupChar = scanner->previous;
        reportCurrentChar(scanner);
    } while(isCurrentNotEOFOrReadAndSetCurrentChar(scanner));

    setAndReportPreviousType(scanner, NUMBER);

    return backupChar;
}

int threeCharCheck(Scanner* scanner, char* array[], int size) {
    int i;
    int temp = EOF;

    for(i = 0; i < size; i++) {
        if(strlen(array[i]) == 3) {
            if(temp == EOF) temp = getChar(scanner);
            if(scanner->current == array[i][0] && scanner->next == array[i][1] && temp == array[i][2]) {
                reportCurrentChar(scanner);
                reportChar(scanner, scanner->next);
                // readAndSetNextChar(scanner);
                scanner->next = temp;
                reportChar(scanner, scanner->next);
                readAndSetNextChar(scanner);
                return TRUE;
            } else {
                
            }
        }

        else if(strlen(array[i]) == 2) {
            if(scanner->current == array[i][0] && scanner->next == array[i][1]) {
                reportCurrentChar(scanner);
                reportChar(scanner, scanner->next);
                readAndSetNextChar(scanner);
                return TRUE;
            } else {

            }
        }

        else {
            if(scanner->current == array[i][0]) {
                reportCurrentChar(scanner);
                return TRUE;
            }
        }
    }

    if(temp != EOF) {
        scanner->characters--;
        ungetc(temp, scanner->sfile);
    }
    
    return FALSE;
}

int twoCharCheck(Scanner* scanner, char* array[], int size) {
    int i;

    for(i = 0; i < size; i++) {
        if(strlen(array[i]) == 2) {
            if(scanner->current == array[i][0] && scanner->next == array[i][1]) {
                reportCurrentChar(scanner);
                reportChar(scanner, scanner->next);
                readAndSetNextChar(scanner);
                return TRUE;
            } else {

            }
        }

        else {
            if(scanner->current == array[i][0]) {
                reportCurrentChar(scanner);
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

int isMemberPointerOperator(Scanner* scanner) {
    return twoCharCheck(scanner, memberPointerOperators, 4);
}

int isArithmeticOperator(Scanner* scanner) {
    return twoCharCheck(scanner, arithmeticOperators, 13);
}

int isRelationalOperator(Scanner* scanner) {
    return twoCharCheck(scanner, relationalOperators, 6);
}

int isLogicalOperator(Scanner* scanner) {
    return twoCharCheck(scanner, logicalOperators, 3);
}

int isBitwiseOperator(Scanner* scanner) {
    return threeCharCheck(scanner, bitwiseOperators, 11);
}

int main(int argc, char *argv[]) {
    Scanner* scanner;
    char temp;
    scanner = (Scanner*) calloc(sizeof(Scanner), 1);

    if(argc != 3) {
        perror("Invalid arguments");
        exit(1);
    }

    initScanner(scanner, argv[1], argv[2]);
    startNewLine(scanner);

    while( ! isCurrentEOF(scanner) || (readAndSetCurrentChar(scanner)) != EOF) {
        readAndSetNextChar(scanner);

        if(isDoubleQuote(scanner->previous, scanner->current)) {
            readString(scanner);
        }

        else if(isSingleQuote(scanner->previous, scanner->current)) {
            readChar(scanner);
        }

        else if(isSingleLineComment(scanner->current, scanner->next)) {
            stripSingleLineComment(scanner);
        }

        else if(isMultiLineComment(scanner->current, scanner->next)) {
            stripMultiLineComment(scanner);
        }

        else if(isHash(scanner->current)) {
            temp = readPreProcessor(scanner);
            restoreChar(scanner, temp);
        }

        else if(isalpha(scanner->current) || scanner->current == '_') {
            temp = readKeywordIdentifier(scanner);
            restoreChar(scanner, temp);
        }

        else if(isdigit(scanner->current)) {
            temp = readDigit(scanner);
            restoreChar(scanner, temp);
        }

        else if(scanner->previousType == IDENTIFIER && isMemberPointerOperator(scanner)) {
            setAndReportPreviousType(scanner, MEMBER_POINTER_OPERATOR);
        }

        else if(scanner->previousType != KEYWORD && isArithmeticOperator(scanner)) {
            setAndReportPreviousType(scanner, ARITHMETIC_OPERATOR);
        }

        else if(scanner->previousType == IDENTIFIER && isBitwiseOperator(scanner)) {
            setAndReportPreviousType(scanner, BITWISE_OPERATOR);
        }

        else if(scanner->previousType == IDENTIFIER && isRelationalOperator(scanner)) {
            setAndReportPreviousType(scanner, RELATIONAL_OPERATOR);
        }

        else if(scanner->previousType != KEYWORD && isLogicalOperator(scanner)) {
            setAndReportPreviousType(scanner, LOGICAL_OPERATOR);
        }

        else if(isDelimiter(scanner->current)) {
            reportCurrentChar(scanner);
            scanner->previousType = DELIMITER;

            reportPreviousType(scanner);
        }

        else if(isNewLine(scanner->current)) {
            incrementAndReportNewLine(scanner);
            // scanner->previousType = WHITE_SPACE;
        }

        else {
            if( ! isspace(scanner->current)) fprintf(scanner->dfile, "%c", scanner->current);
            // scanner->previousType = WHITE_SPACE;
        }

        swapChars(scanner);
    }

    printf("Total characters: %d\n", scanner->characters);

    reportNewLine(scanner);
    closeScanner(scanner);

    return 0;
}