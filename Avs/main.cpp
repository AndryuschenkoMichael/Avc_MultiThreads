#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <ctime>
#include <semaphore.h>
#include <string>
#include <stdexcept>

int arrSize;
char* *data_base;
sem_t mutex_reader;
sem_t mutex_db;
int count_readers = 0;
const int WORK_TIME = 10000;

char *randomString(int size) {
    char *str = new char[size];
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % strlen(charset);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

//стартовая функция потоков-читателей
void *funcRead(void *param) {
    int rNum = *((int*)param);
    while (clock() < WORK_TIME) {
        //получить случайный индекс
        int number = rand() % arrSize;

        // Закрыть базу данных для записи. (КС)
        sem_wait(&mutex_reader);
        ++count_readers;
        if (count_readers == 1) {
            sem_wait(&mutex_db);
        }
        sem_post(&mutex_reader);

        //прочитать данные из общего массива – критическая секция
        char *info = data_base[number];

        // Открыть базу данных, если нет читателей.
        sem_wait(&mutex_reader);
        --count_readers;
        if (count_readers == 0) {
            sem_post(&mutex_db);
        }
        sem_post(&mutex_reader);

        printf("time = %u ---> Reader %d: Element[%d] -> %s\n", clock(), rNum, number, info) ;
        sleep(1);
    }
    return nullptr;
}

//стартовая функция потоков-писателей
void *funcWrite(void *param) {
    int wNum = *((int*)param);
    while (clock() < WORK_TIME) {
        //получить случайный индекс
        int number = rand() % arrSize;
        //закрыть блокировку для записи
        sem_wait(&mutex_db);
        //изменить элемент общего массива – критическая секция
        data_base[number] = randomString(rand() % 10 + 10);
        char *str = data_base[number];
        //открыть блокировку
        sem_post(&mutex_db);
        fprintf(stdout,"time = %u ---> Writer %d: Element[%d] = %s\n", clock(), wNum, number, str) ;
        sleep(2);
    }
    return nullptr;
}

void errMessage1() {
    printf("incorrect command line!\n"
           "  Waited:\n"
           "     command -f infile \n"
           "  Or:\n"
           "     command -n number\n");
}

void errMessage2() {
    printf("incorrect qualifier value!\n"
           "  Waited:\n"
           "     command -f infile\n"
           "  Or:\n"
           "     command -n number\n");
}

void initializeInput(FILE *file) {
    for (int i = 0; i < arrSize; ++i) {
        char* str = new char[256];
        if (fscanf(file, "%s", str) == EOF) {
            throw std::invalid_argument("Incorrect name.");
        }

        data_base[i] = str;
    }
}

void initializeRandom() {
    for (int i = 0; i < arrSize; ++i) {
        data_base[i] = randomString(rand() % 10 + 10);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        errMessage1();
        return 1;
    }

    FILE *file;

    if (!strcmp(argv[1], "-f")) {
        file = fopen(argv[2], "r");
        fscanf(file, "%d", &arrSize);

        if (!(1 <= arrSize && arrSize <= 100)) {
            printf("Incorrect number of count = %d", arrSize);
            return 3;
        }

        data_base = new char *[arrSize];
        initializeInput(file);
        fclose(file);
    } else if (!strcmp(argv[1], "-n")) {
        arrSize = atoi(argv[2]);
        if (!(1 <= arrSize && arrSize <= 100)) {
            printf("Incorrect number of count = %d", arrSize);
            return 3;
        }

        data_base = new char *[arrSize];
        initializeRandom();
    } else {
        errMessage2();
        return 2;
    }

    sem_init(&mutex_reader, 0, 1);
    sem_init(&mutex_reader, 0, 1);

    //создание четырех потоков-читателей
    pthread_t threadR[4] ;
    int readers[4];
    for (int i=0 ; i < 4 ; i++) {
        readers[i] = i+1;
        pthread_create(&threadR[i], NULL, funcRead, (void*)(readers+i)) ;
    }

    //создание трех потоков-писателей
    pthread_t threadW[3] ;
    int writers[3];
    for (int i=0 ; i < 3 ; i++) {
        writers[i] = i+1;
        pthread_create(&threadW[i], NULL, funcWrite, (void*)(writers+i)) ;
    }

    //пусть главный поток будет потоком-писателем
    int mNum = 0;
    funcWrite((void*)&mNum) ;

    return 0;
}
