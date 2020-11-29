#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <omp.h>
#include "Util.h"
#include "Cipher.h"


#define countPerTask 10000


// структура для храниения результата выполнения потока
struct ThreadResult {
    bool success;
    int wrongValue;
};


struct ThreadResult result = {.success=true};

int* encoded; // закодированные данные -- числа
char* decoded; // декодированные данные -- символы (c-string)


// выходит из потока с ошибкой
void reportError(int value) {
    result.success = false;
    result.wrongValue = value;
}

// создаёт задачи
void createTasks(int count) {
    for (int i = 0; i < count; i += countPerTask) {
        int index = i;
        #pragma omp task
        {
            int firstIndex = index;
            int lastIndex = min(index + countPerTask, count);
            decode(encoded, decoded, firstIndex, lastIndex);

            int numberOfThread = omp_get_thread_num();
            int countOfThreads = omp_get_num_threads();
            #pragma omp critical
            {
                printf("Thread %d / %d finished task %d\n", numberOfThread, countOfThreads, index);
            }
        }
    }
}


int main(int argc, char** argv) {
    // количество символов
    int count;
    // считываем исходные данные
    if (!readCipherData(&count, &encoded, &decoded)) {
        pause();
        return 1;
    }

    // запускаем секцию с несколькими потоками
    #pragma omp parallel
    {
        //  в одном потоке создаём задачи
        #pragma omp single
        {
            createTasks(count);
        }
        // ждём, пока все задачи завершатся
        #pragma omp taskwait
    }

    // проверяем флаг ошибки
    if (!result.success) {
        printf("Invalid value: %d\n", result.wrongValue);
        pause();
        return -1;
    }


    printf("Decoded message: ");
    // выводим результат
    puts(decoded);

    // ждём ввода пользователя
    pause();


    // очищаем память
    free(encoded);
    free(decoded);

    return 0;
}
