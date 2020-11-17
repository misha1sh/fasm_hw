#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include "Util.h"
#include "Cipher.h"
#include "TasksBag.h"

#define threadsCount 8
#define countPerTask 1000


// структура для храниения результата выполнения потока
struct ThreadResult {
    bool success;
    int wrongValue;
};


struct TasksBag tasksBag; // портфель задач

int* encoded; // закодированные данные -- числа
char* decoded; // декодированные данные -- символы (c-string)

// основная функция для потоков
void* runTaskLoop(void* args) {
    while (true) {
        struct Task task;
        if (!getNextTask(&tasksBag, &task)) {
            break; // если задачи в портфеле кончились, то выходим
        }

        decode(encoded, decoded, task.firstIndex, task.lastIndex);
    }

    struct ThreadResult* res = malloc(sizeof(struct ThreadResult));
    res->success = true;
    return res;
}

// выходит из потока с ошибкой
void reportError(int value) {
    // очищаем очередь, потому что программа всё равно завершится с ошибкой
    clearTasksBag(&tasksBag);

    // выходим из потока с неудачей
    struct ThreadResult* res = malloc(sizeof(struct ThreadResult));
    res->success = false;
    res->wrongValue = value;
    pthread_exit((void*)res);
}

// создаёт задачи
void createTasks(struct TasksBag* _tasksBag, int count) {
    for (int i = 0; i < count; i += countPerTask) {
        struct Task task;
        task.firstIndex = i;
        task.lastIndex = min(i + countPerTask, count);
        addTask(_tasksBag, task);
    }
}

// создаёт потоки
void createThreads(pthread_t* _threads) {
    for (int i = 0; i < threadsCount; i++) {
        pthread_create(_threads + i, NULL, runTaskLoop, NULL);
    }
}

// дожидается пока потоки отработают. возвращает false, если хотя бы один из потоков
// завершился с ошибкой
bool joinThreads(pthread_t* _threads) {
    for (int i = 0; i < threadsCount; i++) {
        struct ThreadResult* result;
        pthread_join(_threads[i], (void **) &result);
        if (!result->success) {
            printf("Invalid value: %d\n", result->wrongValue);
            return false;
        }
        free(result);
    }
    return true;
}

int main(int argc, char** argv) {
    // количество символов
    int count;
    // считываем исходные данные
    if (!readCipherData(&count, &encoded, &decoded)) {
        pause();
        return 1;
    }

    // создаём портфель задач
    initTasksBag(&tasksBag, (count + countPerTask - 1) / countPerTask);
    createTasks(&tasksBag, count);

    // создаём потоки
    pthread_t threads[threadsCount];
    createThreads(threads);

    // запускаем потоки
    if (!joinThreads(threads)) {
        pause();
        return 1;
    }

    printf("Decoded message: ");
    // выводим результат
    puts(decoded);
    // ждём ввода пользователя
    pause();

    // очищаем память
    destroyTasksBag(&tasksBag);
    free(encoded);
    free(decoded);

    return 0;
}

