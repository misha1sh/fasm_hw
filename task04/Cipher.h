#ifndef CIPHER_H
#define CIPHER_H

#include <stdio.h>
#include <stdlib.h>

char cipherTable[255];

void reportError(int value);

// расшифровывает данные с промежутка [firstIndex, lastIndex)
void decode(int* encoded, char* decoded, int firstIndex, int lastIndex) {
    for (int i = firstIndex; i < lastIndex; i++) {
        int value = encoded[i];
        if (value > 256 || value < 0 || cipherTable[value] == 0) {
            reportError(value);
        }
        decoded[i] = cipherTable[value];
    }
}

// читает таблицу шифрования
bool readCipherTable() {
    int cipherCount = 0;
    printf("Count of cipher entries: ");
    if (scanf("%d", &cipherCount) != 1) {
        printf("Unable to read count of cipher entries\n");
        return false;
    }
    if (cipherCount < 0 || cipherCount > 256) {
        printf("Incorrect count of cipher entries\n");
        return false;
    }

    char tmp[100];
    printf("Cipher entries (in format '<character> = <number>\\n'):\n");
    for (int i = 0; i < cipherCount; i++) {
        char c;
        int number;
        scanf("%[\n ]", tmp); // пропускаем пробелы и переносы строк
        if (scanf("%c%[= ]", &c, tmp) != 2 || scanf("%d", &number) != 1) {
            printf("Unable to read cipher entry\n");
            return false;
        }
        if (number < 0 || number > 255) {
            printf("Incorrect number: %d. Number should be between 0 and 255\n", number);
            return false;
        }
        if (cipherTable[number] != 0) {
            printf("Repeating entries in cipher table");
            return false;
        }
        cipherTable[number] = c;
    }

    return true;
}

// читает данные, которые нужно расшифровать
bool readEncoded(int count, int* encoded) {
    printf("Encoded characters: (in format '<number1> <number2> ... <numberN>'\n");
    for (int i = 0; i < count; i++) {
        if (scanf("%d", encoded + i) != 1) {
            printf("Unable to read %d encoded\n", i + 1);
            return false;
        }
    }

    return true;
}

// читает данные, которые нужно расшифровать и таблицу шифрования
bool readCipherData(int* count, int** encoded, char** decoded) {
    printf("Count of encoded characters: ");
    if (scanf("%d", count) != 1) {
        printf("Unable to read encoded count\n");
        return false;
    }
    if (*count < 0 || *count > 5e8) {
        printf("Incorrect encoded count\n");
        return false;
    }

    *encoded = malloc(*count * sizeof(int));
    *decoded = malloc((*count + 1) * sizeof(char));
    (*decoded)[*count] = 0; // записываем в конец нуль-турминатор

    if (!readEncoded(*count, *encoded)) {
        return false;
    }

    return readCipherTable();
}

#endif