#include <stdio.h>
#include <ctype.h> // isDigit isAlpha
#include <mem.h> // strCMP CHR
#include <stdlib.h> // malloc , exit , free

#include "headers/constants.h"
#include "headers/executor.h"
#include "headers/main.h"

/*
 * Подсчет выражений происходит методом рекурсивного спуска.
 * Считается по порядку приоритета:
 * level2 - просихожит + или - двух частей
 * level3 - происходит *, / или % двух частей
 * level4 - вычисление унарной операции
 * level5 - вычисление скобок
 * */

void start(char *program, struct infoLabels *infoLabels, struct infoVariables *infoVars, char *fileResult) {
    struct lexem *token = (struct lexem *) malloc(sizeof(struct lexem));
    token->pointToProgram = program;
    token->line = 1;

    struct infoGosub *infoGosub = (struct infoGosub *) malloc(sizeof(struct infoGosub));
    infoGosub->gIndex = 0;

    do {
        getToken(token);
        //Проверка на присваивание
        if (token->type == VARIABLE) {
            comeBack(token);
            assignment(token, infoVars);
        }

        //Проверка на команду
        if (token->type == COMMAND) {
            switch (token->id) {
                case PRINT:
                    basicPrint(token, infoVars, fileResult);
                    break;
                case INPUT:
                    basicInput(token, infoVars);
                    break;
                case IF:
                    basicIf(token, infoLabels, infoVars, infoGosub, fileResult);
                    break;
                case GOTO:
                    basicGoto(token, infoLabels);
                    break;
                case GOSUB:
                    basicGosub(infoGosub, token, infoLabels);
                    break;
                case RETURN:
                    basicReturn(infoGosub, token);
                    break;
                case END:
                    exit(0);
                default:
                    break;
            }
        }
    } while (token->id != FINISHED);

    free(token);
    free(infoGosub);
    free(infoLabels);
    free(infoVars);
}

void getToken(struct lexem *token) {
    token->id = 0;
    token->type = 0;

    char *program = token->pointToProgram; // Здесь еще указатель на текущую  лексему

    //Пропускаем пробелы
    while (isWhite(*program))
        program++;

    //Проверка закончился ли файл интерпретируемой программы
    if (*program == '\0') {
        token->id = FINISHED;
        token->type = DELIMITER;
        token->pointToProgram = program;// Меняю положение указателя в памяти уже на следующую лексему
        return;
    }

    //Проверка на конец строки программы
    if (*program == '\n') {
        program++;
        token->id = EOL;
        token->type = DELIMITER;
        token->pointToProgram = program;// Меняю положение указателя в памяти уже на следующую лексему
        token->line++;
        return;
    }

    char *tempStart = program;

    //Проверка на разделитель
    if (strchr("+-*/%=:,()><", *program)) {
        if (*program != '<' && *program != '>'){
            token->name = mallocAndCopy(tempStart, 1);
            program++;
        }
        if (*program == '<') {
            program++;
            if (*program == '>' || *program == '=') {
                token->name = mallocAndCopy(tempStart, 2);
                program++;
            } else {
                token->name = mallocAndCopy(tempStart, 1);
            }
        }
        if (*program == '>') {
            program++;
            if (*program == '=') {
                token->name = mallocAndCopy(tempStart, 2);
                program++;
            } else {
                token->name = mallocAndCopy(tempStart, 1);
            }
        }
        token->type = DELIMITER;
        token->pointToProgram = program;// Меняю положение указателя в памяти уже на следующую лексему
        return;
    }

    //Проверяем на кавычки
    if (*program == '"') {
        program++;
        tempStart++;
        int counter = 0;
        while (*program != '"' && *program != '\n') {
            program++;
            counter++;
        }
        if (*program == '\n')
            printErrorLine("Unpaired quotes", token->line);
        token->name = mallocAndCopy(tempStart, counter);
        program++;
        token->type = QUOTE;
        token->pointToProgram = program;// Меняю положение указателя в памяти уже на следующую лексему
        return;
    }

    //Проверка на число
    if (isdigit(*program)) {
        int counter = 0;
        while (!isDelim(*program)) {
            program++;
            counter++;
        }
        token->name = mallocAndCopy(tempStart, counter);
        token->type = NUMBER;
        token->pointToProgram = program; // Меняю положение указателя в памяти уже на следующую лексему
        return;
    }

    //Переменная или команда?
    if (isalpha(*program)) {
        int counter = 0;
        while (!isDelim(*program)) {
            program++;
            counter++;
        }
        token->name = mallocAndCopy(tempStart, counter);
        token->id = getIntCommand(token->name); //Получение внутреннего представления команды
        if (!token->id) {
            token->type = VARIABLE;
        } else
            token->type = COMMAND;
        token->pointToProgram = program;// Меняю положение указателя в памяти уже на следующую лексему
        return;
    }
    printError("Syntax error!!!");
}

char *mallocAndCopy(char *source, int steps) {
    char *resultPointer = (char *) malloc((size_t) sizeof(char) * (steps + 1));
    char *tempPointer = resultPointer;
    for (int i = 0; i < steps; i++) {
        *tempPointer = *source;
        tempPointer++;
        source++;
    }
    *tempPointer = '\0';
    return resultPointer;
}

int isWhite(char c) {
    if (c == ' ' || c == '\t') return 1;
    else return 0;
}

int isDelim(char c) {
    if (strchr(" !;,+-<>\'/*%=()\"", c) || c == '\r' || c == '\n')
        return 1;
    return 0;
}

int getIntCommand(char *command) {
    struct command {
        char name[7]; // не больше 10 симовлов
        int token_int;
    } tableCommand[] = {
            "PRINT", PRINT,
            "INPUT", INPUT,
            "IF", IF,
            "THEN", THEN,
            "ELSE", ELSE,
            "ENDFI", ENDFI,
            "GOTO", GOTO,
            "GOSUB", GOSUB,
            "RETURN", RETURN,
            "END", END};
    //Поиск лексемы в таблице операторов
    for (int i = 0; *tableCommand[i].name; i++) {
        if (!strcmp(tableCommand[i].name, command))
            return tableCommand[i].token_int;
    }
    return 0;
}

void comeBack(struct lexem *token) {
    char *t = token->name;
    char *program = token->pointToProgram;
    while (*t != '\0') {
        t++;
        program--;
    }
    if (token->type == QUOTE)
        program -= 2;
    token->pointToProgram = program;
}

struct variable *findVariable(struct infoVariables *infoVars, char *name) {
    int i = 1;
    struct variable *temp = infoVars->vars;
    while (i <= infoVars->count_vars) {
        if (!strcmp(name, temp->name)) {
            return temp;
        }
        i++;
        temp++;
    }
    return NULL; // не найдена, либо вообще пустой блок
}

struct variable *addVariable(struct infoVariables *infoVars, char *name) {
    infoVars->count_vars++;
    infoVars->vars = (struct variable *) realloc(infoVars->vars, sizeof(struct variable) * infoVars->count_vars);
    struct variable *temp = infoVars->vars;

    int i = 1;
    while (i < infoVars->count_vars) {
        temp++;
        i++;
    }

    temp->name = name;
    return temp;
}

void assignment(struct lexem *token, struct infoVariables *infoVars) {
    int value;
    getToken(token); //Получаем имя переменной
    struct variable *var = findVariable(infoVars, token->name); //Точно не NULL
    getToken(token); //Считываем равно
    getExpression(&value, token, infoVars);
    var->value = value;
}

void getExpression(int *result, struct lexem *token, struct infoVariables *infoVars) {
    getToken(token);
    level2(result, token, infoVars);
    comeBack(token);
}

void level2(int *result, struct lexem *token, struct infoVariables *infoVars) {
    char operation;
    level3(result, token, infoVars);

    int hold;
    while ((operation = *token->name) == '+' || operation == '-') {
        getToken(token);
        level3(&hold, token, infoVars);
        arithmetic(operation, result, &hold);
    }
}

void level3(int *result, struct lexem *token, struct infoVariables *infoVars) {
    char operation;
    level4(result, token, infoVars);

    int hold;
    while ((operation = *token->name) == '/' || operation == '%' || operation == '*') {
        getToken(token);
        level4(&hold, token, infoVars);
        arithmetic(operation, result, &hold);
    }
}

void level4(int *result, struct lexem *token, struct infoVariables *infoVars) {
    char operation = 0;
    if ((token->type == DELIMITER) && *token->name == '+' || *token->name == '-') {
        operation = *token->name;
        getToken(token);
    }
    level5(result, token, infoVars);
    if (operation)
        unary(operation, result);
}

void level5(int *result, struct lexem *token, struct infoVariables *infoVars) {
    if ((*token->name == '(') && (token->type == DELIMITER)) {
        getToken(token);
        level2(result, token, infoVars);
        getToken(token);
    } else
        value(result, token, infoVars);
}

void value(int *result, struct lexem *token, struct infoVariables *infoVars) {
    switch (token->type) {
        case VARIABLE:
            *result = findVariable(infoVars, token->name)->value;
            getToken(token);
            return;
        case NUMBER:
            *result = atoi(token->name);
            getToken(token);
            return;
        default:
            printError("Syntax error!");
    }
}

void unary(char o, int *r) {
    if (o == '-') *r = -(*r);
}

void arithmetic(char o, int *r, int *h) {
    int t;
    switch (o) {
        case '-':
            *r = *r - *h;
            break;
        case '+':
            *r = *r + *h;
            break;
        case '*':
            *r = *r * *h;
            break;
        case '/':
            *r = (*r) / (*h);
            break;
        case '%':
            t = (*r) / (*h);
            *r = *r - (t * (*h));
            break;
        default:
            break;
    }
}

void basicPrint(struct lexem *token, struct infoVariables *infoVars, char *fileResult) {
    FILE *file;
    file = fopen(fileResult, "a");

    int answer;
    do {
        getToken(token);
        if (token->type == QUOTE) {
            fputs(token->name, file);
            printf(token->name);
        } else {
            comeBack(token);
            getExpression(&answer, token, infoVars);
            fprintf(file, "%d", answer);
            printf("%d", answer);
        }
        getToken(token); //Считывается запятая
    } while (*token->name == ',');
    fputc('\n', file);
    printf("\n");
    fclose(file);
}

void basicInput(struct lexem *token, struct infoVariables *infoVars) {
    getToken(token);
    if (token->type == QUOTE) {
        printf(token->name);
        getToken(token); //Пропускаем запятую
        getToken(token);
    } else
        printf("Write data: ");
    struct variable *var = findVariable(infoVars, token->name);
    int i;
    if (!scanf("%d", &i))
        printErrorLine("Incorrect input data!", token->line);   //Чтение входных данных
    var->value = i;
}

void basicGoto(struct lexem *token, struct infoLabels *infoLabels) {
    char *location;
    getToken(token); //Получаем метку
    location = findLabel(infoLabels, token->name);
    if (location == '\0')
        printErrorLine("Undefined label", token->line);
    else
        token->pointToProgram = location;
}

void basicGosub(struct infoGosub *infoGosub, struct lexem *token, struct infoLabels *infoLabels) {
    char *location;
    getToken(token); //Считываение метки
    location = findLabel(infoLabels, token->name);
    if (location == '\0')
        printErrorLine("Undefined label", token->line); //Метка не определена
    else {
        gPush(infoGosub, token->pointToProgram); //Запомним место, куда вернемся
        token->pointToProgram = location; //Старт программы с указанной точки
    }
}

void basicReturn(struct infoGosub *infoGosub, struct lexem *token) {
    token->pointToProgram = gPop(infoGosub);
}

void basicIf(struct lexem *token, struct infoLabels *infoLabels, struct infoVariables *infoVars, struct infoGosub *infoGosub, char *fileResult) {
    int left, right, condition;
    getExpression(&left, token, infoVars);
    getToken(token); //Получаем оператор
    char operation = *token->name;
    char *temp = token->name;
    getExpression(&right, token, infoVars);
    condition = 0;
    switch (operation) {
        case '=':
            if (left == right) condition = 1;
            break;
        case '<':
            temp++;
            if (*temp == '>') {
                if (left != right)
                    condition = 1;
                break;
            }
            if (*temp == '=') {
                if (left <= right)
                    condition = 1;
                break;
            }
            if (left < right) condition = 1;
            break;
        case '>':
            if (*(++temp) == '=') {
                if (left >= right)
                    condition = 1;
                break;
            }
            if (left > right) condition = 1;
            break;
        default:
            break;
    }
    if (condition) {
        getToken(token); //Считываем THEN
        do {
            getToken(token);
            //Проверка на присваивание
            if (token->type == VARIABLE) {
                comeBack(token);
                assignment(token, infoVars);
            }

            //Проверка на команду
            if (token->type == COMMAND) {
                switch (token->id) {
                    case PRINT:
                        basicPrint(token, infoVars, fileResult);
                        break;
                    case INPUT:
                        basicInput(token, infoVars);
                        break;
                    case IF:
                        basicIf(token, infoLabels, infoVars, infoGosub, fileResult);
                        token->id = 0;
                        break;
                    case GOTO:
                        basicGoto(token, infoLabels);
                        return;
                    case GOSUB:
                        basicGosub(infoGosub, token, infoLabels);
                        break;
                    case RETURN:
                        if (infoGosub->gIndex == 0)
                            return;
                        basicReturn(infoGosub, token);
                        break;
                    default:
                        break;
                }
            }
        } while (token->id != ELSE && token->id != ENDFI);
        if (token->id == ENDFI)
            return;

        //Пропускаем ELSE
        int countIf = 0;
        do {
            getToken(token);
            if (token->id == IF)
                countIf++;
            if (token->id == ENDFI && countIf != 0)
                countIf--;
        } while (token->id != ENDFI || countIf != 0);
    } else {
        //Пропускаем THEN
        int countIf = 0;
        do {
            getToken(token);
            if (token->id == IF)
                countIf++;
            if (token->id == ENDFI && countIf != 0)
                countIf--;
        } while ((token->id != ENDFI || countIf != 0) && (token->id != ELSE || countIf != 0));
        if (token->id == ENDFI)
            return;
        do {
            getToken(token);
            //Проверка на присваивание
            if (token->type == VARIABLE) {
                comeBack(token);
                assignment(token, infoVars);
            }

            //Проверка на команду
            if (token->type == COMMAND) {
                switch (token->id) {
                    case PRINT:
                        basicPrint(token, infoVars, fileResult);
                        break;
                    case INPUT:
                        basicInput(token, infoVars);
                        break;
                    case IF:
                        basicIf(token, infoLabels, infoVars, infoGosub, fileResult);
                        token->id = 0;
                        break;
                    case GOTO:
                        basicGoto(token, infoLabels);
                        return;
                        break;
                    case GOSUB:
                        basicGosub(infoGosub, token, infoLabels);
                        break;
                    case RETURN:
                        if (infoGosub->gIndex == 0)
                            return;
                        basicReturn(infoGosub, token);
                        break;
                    default:
                        break;
                }
            }
        } while (token->id != ENDFI);
    }
}

char *findLabel(struct infoLabels *infoLabels, char *label) {
    int i = 1;
    struct label *temp = infoLabels->labels;
    while (i <= infoLabels->countLabels) {
        if (!strcmp(label, temp->name)) {
            return temp->location;
        }
        i++;
        temp++;
    }
    return NULL;
}

void gPush(struct infoGosub *infoGosub, char *pointer) {
    infoGosub->gIndex++;
    if (infoGosub->gIndex == 1)
        infoGosub->gStackStart = (struct Gosub *) malloc(sizeof(struct Gosub));
    else
        infoGosub->gStackStart = (struct Gosub *) realloc(infoGosub->gStackStart,
                                                          sizeof(struct Gosub) * infoGosub->gIndex);
    infoGosub->gStackEnd = infoGosub->gStackStart;

    int i = 1;
    while (i < infoGosub->gIndex) {
        infoGosub->gStackEnd++;
        i++;
    }
    infoGosub->gStackEnd->location = pointer;
}

char *gPop(struct infoGosub *infoGosub) {
    if (infoGosub->gIndex == 0) {
        printError("Undefined label");
        return '\0';
    }
    char *result = infoGosub->gStackEnd->location;
    infoGosub->gIndex--;
    infoGosub->gStackEnd--;
    return result;
}