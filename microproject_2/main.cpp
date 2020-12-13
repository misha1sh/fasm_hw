#include <iostream>
#include <sstream>
#include <pthread.h>
#include <semaphore.h>
#include <atomic>
#include <chrono>


#ifdef WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif
// кроссплатформенная фукнция для засыпания на определённое количество миллисекунд
void sleepForMS(unsigned int sleepMs)
{
#ifdef WINDOWS
    Sleep(sleepMs);
#else
    usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#endif
}


// количество потоков-курильщиков
const int SMOKERS_COUNT = 3;

// виды ресурсов, необходимых для курения
enum ResourceType {
    TOBACCO = 0,
    PAPER = 1,
    MATCHES = 2,
    RESOURCES_COUNT // стандартный трюк для хранения количества элементов в enum
};

// функция для представления ресурса в строковом виде
std::string getResourceName(const ResourceType& type) {
    switch (type) {
        case TOBACCO:
            return "Табак";
        case PAPER:
            return "Бумага";
        case MATCHES:
            return "Спички";
    }
    return "Неизвестный ресурс";
}


// время на курение
int smokeTime;
// количество итераций выкладывания на стол товаров
int iterationsCount;

// флаг, чтобы сообщать потокам об необходимости остановиться
std::atomic<bool> keepRunning;

// массив курильщиков
pthread_t smokers[SMOKERS_COUNT];
int smokerId[SMOKERS_COUNT];

// поставщик
pthread_t provider;

// семафоры для ресурсов, которые уже есть у курильщиков
// нужны, чтобы курильщик мог ждать, когда у поставщика появятся
// другие два вида ресурсов
sem_t resourcesWait[RESOURCES_COUNT];
// семафор для ожидания поставщиком курильщика
sem_t smokeWait;

// семафор для синхронизации вывода
sem_t coutWait;
// специальный вспомогательный класс для синхронизации вывода
class CoutThreadSafe: public std::ostringstream {
public:
    CoutThreadSafe() = default;
    ~CoutThreadSafe() {
        sem_wait(&coutWait);
        std::cout << this->str();
        sem_post(&coutWait);
    }
};
// макрос, чтобы классом можно было пользововаться так же, как и cout
#define coutThreadSafe CoutThreadSafe{}


// Основной метод для курильщика
void* Smoker(void* param) {
    int smokerId = *reinterpret_cast<int*>(param);
    std::string resourceName = getResourceName(static_cast<ResourceType>(smokerId));
    coutThreadSafe << "Запустился Курильщик#" << smokerId << " у которого есть "
              << resourceName  << std::endl;

    while (keepRunning) {
        int semResult = sem_wait(&resourcesWait[smokerId]);
        if (semResult != 0 || !keepRunning){
            // если семафор вернул ошибку или пора останавливаться, значит нужно остановить работу
            break;
        }
        coutThreadSafe << "Курильщик#" << smokerId << "(" << resourceName
                  << ") забрал со стола ресурсы и начал курить" << std::endl;
        sleepForMS(smokeTime);

        coutThreadSafe << "Курильщик#" << smokerId << " докурил" << std::endl;
        sem_post(&smokeWait);
    }
    coutThreadSafe << "Поток для Курильщика#" << smokerId << " завершил работу" << std::endl;
    return nullptr;
}

// Основной метод для поставщика
void* Provider(void* param) {
    for (int iterations = 0; iterations < iterationsCount; iterations++) {
        if (!keepRunning) {
            break;
        }
        int otherResource = rand() % RESOURCES_COUNT;
        for (int i = 0; i < RESOURCES_COUNT; i++) {
            if (i != otherResource) {
                coutThreadSafe << "Поставщик выкладывает на стол " << getResourceName(static_cast<ResourceType>(i)) << std::endl;
            }
        }
        // будим курильщика, у которого есть нехватающий ресурс
        sem_post(&resourcesWait[otherResource]);

        // ждём, пока курильщик докурит
        sem_wait(&smokeWait);
    }

    coutThreadSafe << "Поток для Поставщика завершил работу" << std::endl;
    return nullptr;
}

// функция для вывода вспомогательного сообщения
void printHelpMessage() {
    std::cout << "./program <smokeTime> <iterationsCount>\n";
    std::cout << "smokeTime -- время на курение\n";
    std::cout << "iterationsCount -- суммарное количество выкладываний на стол компонентов" << std::endl;
}

// функция для считывания консольных аргументов
// возвращает true, если аргументы были прочитаны успешно
bool parseArgs(int argc, char** argv) {
    if (argc == 3) {
        try {
            smokeTime = std::stoi(argv[1]);
            iterationsCount = std::stoi(argv[2]);
        } catch (const std::invalid_argument& exception) {
            std::cerr << "Некорректно задан числовой аргумент" << std::endl;
            return false;
        } catch (const std::out_of_range& exception) {
            std::cerr << "Заданный числовой аргумент слишком большой или слишком маленький" << std::endl;
            return false;
        }
        if (smokeTime < 0 || smokeTime > 1000000) {
            std::cerr << "Задано некорректное время на курение" << std::endl;
            return false;
        }
        if (iterationsCount <= 0) {
            std::cerr << "Задано некорректное количество итераций" << std::endl;
            return false;
        }
        return true;
    } else {
        std::cerr << "Задано некорректное количество аргументов" << std::endl;
        return false;
    }
}

// инициализирует семафоры
void initSemaphores() {
    for (auto & i : resourcesWait) {
        sem_init(&i, 0, 0);
    }
    sem_init(&smokeWait, 0, 0);
    sem_init(&coutWait, 0, 1); // 1, поскольку изначально вывод свободен
}


// фукнция для создания курильщиков
void createSmokers() {
    keepRunning = true;
    for (int i = 0; i < SMOKERS_COUNT; i++) {
        smokerId[i] = static_cast<ResourceType>(i);
        pthread_create(&smokers[i], nullptr, Smoker, &smokerId[i]);
    }
}

// запускает поток с поставщиком
void createProvider() {
    pthread_create(&provider, nullptr, Provider, nullptr);
}

// останавливает потоки с курильщиками
void stopSmokers() {
    // сообщаем потокам, что пора остановить работу
    keepRunning = false;
    // чтобы ожидающие потоки перестали ожидать, разблокируем для них семафоры
    for (auto &sem : resourcesWait) {
        sem_post(&sem);
    }
    sem_post(&smokeWait);

    // ждём, пока все потоки завершат работу
    for (pthread_t smoker : smokers) {
        pthread_join(smoker, nullptr);
    }
}

// очищает семафоры
void destroySemaphores() {
    // разрушаем семафоры
    for (auto &sem : resourcesWait) {
        sem_destroy(&sem);
    }
    sem_destroy(&smokeWait);
    sem_destroy(&coutWait);
}

// основной метод программы
int main(int argc, char** argv) {
    if (!parseArgs(argc, argv)) {
        printHelpMessage();
        return -1;
    }

    initSemaphores();

    srand(time(nullptr));

    // создаём потоки
    createSmokers();

    // создаём поставщика
    createProvider();

    // ждём, пока поставщик завершит работу
    pthread_join(provider, nullptr);

    // останавливаем потоки
    stopSmokers();

    std::cout << "Программа успешно завершила работу" << std::endl;

    return 0;
}
