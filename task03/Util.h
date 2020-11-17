#ifndef UTIL_H
#define UTIL_H

// ждёт от пользователя ввода
void pause() {
#ifdef _WIN32
    system("pause");
#else
    system("read");
#endif
}

// находит минимальное число из двух
int min(int x, int y) {
    if (x < y) {
        return x;
    }
    return y;
}

#endif