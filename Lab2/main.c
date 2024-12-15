#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define TRIANGLES_COUNT 10

int toInt(const char* argv) {
    int res = 0, i = 0;
    while (argv[i] != '\0') {
        res = res * 10 + argv[i] - '0';
        i++;
    }
    return res;
}

typedef struct {
    int x;
    int y;
    int z;
} Point;

typedef struct {
    Point a;
    Point b;
    Point c;
} Triangle;

typedef struct {
    Triangle *triangles;
    int start;
    int end;
    double max_area;
    Triangle max_triangle;
} ThreadData;

void fillTriangles(Triangle *trArr) {
    for (int i = 0; i < TRIANGLES_COUNT; i++) {
        trArr->a.x = rand() % 100;
        trArr->a.y = rand() % 100;
        trArr->a.z = rand() % 100;
        trArr->c.x = rand() % 100;
        trArr->c.y = rand() % 100;
        trArr->c.z = rand() % 100;
        trArr->b.x = rand() % 100;
        trArr->b.y = rand() % 100;
        trArr->b.z = rand() % 100;
    }
}

void printTriangles(Triangle *trArr) {
    for (int i = 0; i < TRIANGLES_COUNT; i++) {
        printf("Triangle #%d:\n(%d; %d; %d) - (%d; %d; %d) - (%d; %d; %d)\n\n", i + 1, trArr->a.x, trArr->a.y, trArr->a.z, trArr->b.x, trArr->b.y, trArr->b.z, trArr->c.x, trArr->c.y, trArr->c.z);
    }
}

double triangleArea(Triangle triangle) {
    double ABx = triangle.b.x - triangle.a.x;
    double ABy = triangle.b.y - triangle.a.y;
    double ABz = triangle.b.z - triangle.a.z;

    // Вектор AC
    double ACx = triangle.c.x - triangle.a.x;
    double ACy = triangle.c.y - triangle.a.y;
    double ACz = triangle.c.z - triangle.a.z;

    // Векторное произведение AB и AC
    double cross_x = ABy * ACz - ABz * ACy;
    double cross_y = ABz * ACx - ABx * ACz;
    double cross_z = ABx * ACy - ABy * ACx;

    // Площадь треугольника равна половине длины вектора, полученного от векторного произведения
    return 0.5 * sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
}

void* calculateArea(void* arg) {

    return NULL;
}

int main(int argc, char *argv[]) {
    // if (argc != 2) {
    //     const char msg[] = "Provide thread number\n";
    //     write(STDERR_FILENO, msg, sizeof(msg));
    //     exit(EXIT_FAILURE);
    // }

    Triangle triangleArray[TRIANGLES_COUNT];
    fillTriangles(triangleArray);
    printTriangles(triangleArray);


    const int num_threads = toInt(argv[1]);
    pthread_t threads[num_threads];
    ThreadData threadData[num_threads];

    int perThread = TRIANGLES_COUNT / num_threads;

    for (int i = 0; i < num_threads; i++) {
        threadData[i].triangles = triangleArray;
        threadData[i].start = i * perThread;
        threadData[i].end = (i + 1) * perThread;
        threadData[i].max_area = 0;
        pthread_create(&threads[i], NULL, calculateArea, (void*)&threadData[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}