#ifndef TASKS_BAG_H
#define TASKS_BAG_H

#include <stdlib.h>

struct Task {
    int firstIndex;
    int lastIndex;
};

// класс для синхронного портфеля задач
struct TasksBag {
    struct Task* tasks;
    int curTask;
    int tasksCount;
    pthread_mutex_t tasksMutex;
};

// инициализирует портфель задач
void initTasksBag(struct TasksBag* _tasksBag, int maxTasksCount) {
    _tasksBag->tasks = malloc(maxTasksCount * sizeof(struct Task));
    _tasksBag->curTask = 0;
    _tasksBag->tasksCount = 0;
    pthread_mutex_init(&_tasksBag->tasksMutex, NULL);
}

// добавляет задачу в портфель задач
void addTask(struct TasksBag* _tasksBag, struct Task task) {
    _tasksBag->tasks[_tasksBag->tasksCount++] = task;
}

// удаляет все задачи из портфеля задач
void clearTasksBag(struct TasksBag* _tasksBag) {
    pthread_mutex_lock(&_tasksBag->tasksMutex);
    _tasksBag->curTask = _tasksBag->tasksCount = 0;
    pthread_mutex_unlock(&_tasksBag->tasksMutex);
}

// деструктор портфеля задач
void destroyTasksBag(struct TasksBag* _tasksBag) {
    free(_tasksBag->tasks);
    pthread_mutex_destroy(&_tasksBag->tasksMutex);
}


// получает следующую задачу из портфеля
bool getNextTask(struct TasksBag* _tasksBag, struct Task* task) {
    pthread_mutex_lock(&_tasksBag->tasksMutex);

    if (_tasksBag->curTask >= _tasksBag->tasksCount) {

        pthread_mutex_unlock(&_tasksBag->tasksMutex);
        return false;
    }
    *task = _tasksBag->tasks[_tasksBag->curTask++];

    pthread_mutex_unlock(&_tasksBag->tasksMutex);
    return true;
}

#endif