#include <stdio.h>
#include <stdlib.h>

#include "headers/analyzer.h"
#include "headers/executor.h"
#include "headers/main.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Interpreter BASIC. The program executes the code written in BASIC.\n\n");
        printf("Use format: <executable file>.exe <input file>.txt <output file>.txt\n\n");
        printf("Example code:\n");
        printf("test = 100\n");
        printf("PRINT \"test = \", test\n");
        printf("END\n\n");
        printf("Created by Kirill Shlyazhko");
        exit(1);
    }

    char *fileName = argv[1]; //Имя файла инпут программы
    char *fileResult = argv[2]; //Имя файла для вывода результатов

    int fileLength = countFileChars(fileName); // Кол-во символов

    char *program = (char*) malloc((size_t) (fileLength + 1));
    if (program == NULL)
        printError("Can't to allocate the memory.");

    //Загрузка программы
    if (!loadProgram(fileName, program))
        exit(1);

    char *p = program; // Указатель на уже записанную в память информацию

    //Анализ текста программы на ошибки

    struct resultByAnalyz resultByAnalyz = analyzing(p);



     // Если вводится имя уже существвующего файла для записи результата, то его надо заменить
    //удаляется существующая запись, и дальше создастся новая.

    remove(fileResult);

    // исполнитель
    start(program, &resultByAnalyz.infoLabels, &resultByAnalyz.infoVars, fileResult);

    free(program);
    return 0;
}

int loadProgram(char *fileName, char *program) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        printError("Can't to read the program file.");
        return 0;
    }
    char *point = program;
    do {
        *point = (char) getc(file);
        point++;
    } while (*(point - 1) != EOF);
    *(point - 1) = '\0';

    fclose(file);
    return 1;
}

int countFileChars(char *fileName) {
    int length = 0;
    FILE *inputFile = fopen(fileName, "r");
    if (!inputFile) {
        printError("Can't to read program file.");
    }
    int ch = getc(inputFile);

    if (ch == EOF){
        printError("File empty");
    }

    while (ch != EOF) {
        length++;
        ch = getc(inputFile);
    }
    fclose(inputFile);
    return length;
}

void printErrorLine(char *error, int line) {
    printf("%s Line: %d", error, line);
    exit(1);
}

void printError(char *error) {
    printf(error);
    exit(1);
}