#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

int toInt(const char* argv) {
    int res = 0, i = 0;
    while (argv[i] != '\0') {
        res = res * 10 + argv[i] - '0';
        i++;
    }
    return res;
}

typedef struct {
    double x, y, z;
} Point;

typedef struct {
    Point *points;
    int numThreads;
    int end;
    int ind;
    double max_area;
} ThreadData;

typedef struct {
    Point a;
    Point b;
    Point c;
} Triangle;

double triangle_area(const Point a, const Point b, const Point c) {
    double ABx = b.x - a.x;
    double ABy = b.y - a.y;
    double ABz = b.z - a.z;

    // Вектор AC
    double ACx = c.x - a.x;
    double ACy = c.y - a.y;
    double ACz = c.z - a.z;

    // Векторное произведение AB и AC
    double cross_x = ABy * ACz - ABz * ACy;
    double cross_y = ABz * ACx - ABx * ACz;
    double cross_z = ABx * ACy - ABy * ACx;

    // Площадь треугольника равна половине длины вектора, полученного от векторного произведения
    return 0.5 * sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
}

void* find_max_area(void* arg) {
    ThreadData *data = (ThreadData*)arg;
    double max_area = 0.0;

    for (int i = data->ind; i < data->end; i += data->numThreads) {
        for (int j = i + 1; j < data->end; j++) {
            for (int k = j + 1; k < data->end; k++) {
                double area = triangle_area(data->points[i], data->points[j], data->points[k]);
                //printf("i: %d | j: %d | k: %d  ||| area: %lf\n", i, j, k, area);
                if (area > max_area) {
                    max_area = area;
                }
            }
        }
    }

    data->max_area = max_area;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: <number of points> <number of threads>\n");
        return -1;
    }

    const int num_points = toInt(argv[1]);
    const int num_threads = toInt(argv[2]);
    Point *points = malloc(num_points * sizeof(Point));

    for (int i = 0; i < num_points; i++) {
        points[i].x = rand() % 100;
        points[i].y = rand() % 100;
        points[i].z = rand() % 100;
    }

    int points_per_thread = num_points / num_threads;

    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    double max_area = 0.0;

    // Creating threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].points = points;
        thread_data[i].numThreads = num_threads;
        thread_data[i].end = num_points;
        thread_data[i].ind = i;
        pthread_create(&threads[i], NULL, find_max_area, &thread_data[i]);
        printf("Thread %d\n", i + 1);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        if (thread_data[i].max_area > max_area) {
            max_area = thread_data[i].max_area;
        }
    }

    printf("Max area: %f\n", max_area);
    free(points);
    return 0;
}
