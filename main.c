#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

typedef struct Node {
    int index;
    struct Node* next;
} Node;

Node* createNode(int index) {
    Node* newNode = (Node*) malloc(sizeof(Node));
    newNode->index = index;
    newNode->next = NULL;
    return newNode;
}

void addNode(Node** head, int index) {
    Node* newNode = createNode(index);
    newNode->next = *head;
    *head = newNode;
}

char* loadPng(const char* filename, int* width, int* height) {
    unsigned char* image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if(error){
      printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    return (image);
}

void writePng(const char* filename, const unsigned char* image, unsigned width, unsigned height) {
    unsigned char* png;
    long unsigned int pngsize;

    int error = lodepng_encode32(&png, &pngsize, image, width, height);
    if(!error) {
       lodepng_save_file(png, pngsize, filename);
    }
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
    free(png);
}

void relative_brightness_of_color(int w, int h, unsigned char *picture, Node** clusters, unsigned char *img1) {
    for (int i = 0; i < w * h; i++) {
        unsigned char brightness = 0.250 * picture[4 * i] + 0.600 * picture[4 * i + 1] + 0.100 * picture[4 * i + 2];
        img1[i] = brightness;  // Заполняем массив img1 яркостью
        if (brightness < 85) {
            addNode(&clusters[0], i);
        } else if (brightness < 170) {
            addNode(&clusters[1], i);
        } else {
            addNode(&clusters[2], i);
        }
    }
}

void gauss_smoothing(int w, int h, unsigned char *picture1, unsigned char *picture2) {
    double kernel[3][3] = {
        {0.09, 0.12, 0.09},
        {0.12, 0.15, 0.12},
        {0.09, 0.12, 0.09}
    };

    for (int i = 1; i < h - 1; i++) {
        for (int j = 1; j < w - 1; j++) {
            double sum = 0;
            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    sum += picture1[w * (i + ki) + (j + kj)] * kernel[ki + 1][kj + 1];
                }
            }
            picture2[w * i + j] = (unsigned char) sum;
        }
    }
}

void apply_color_pattern(int w, int h, unsigned char *picture2, unsigned char *picture4, Node** clusters) {
    unsigned char colors[][3] = {
        {168, 242, 227},  // Красный
        {129, 89, 156},  // Зеленый
        {222, 78, 145}   // Синий
    };

    for (int i = 0; i < 3; i++) {
        Node* current = clusters[i];
        while (current != NULL) {
            int index = current->index;
            picture4[index * 4] = colors[i][0];
            picture4[index * 4 + 1] = colors[i][1];
            picture4[index * 4 + 2] = colors[i][2];
            picture4[index * 4 + 3] = 200;
            current = current->next;
        }
    }
}

int main() {
    char *filename = "skull.png";
    int w, h;
    char *img = loadPng(filename, &w, &h);

    unsigned char *img1 = (unsigned char*)malloc(h * w * sizeof(unsigned char));
    unsigned char *img2 = (unsigned char*)malloc(h * w * sizeof(unsigned char));
    unsigned char *img4 = (unsigned char*)malloc(h * w * 4 * sizeof(unsigned char));

    Node* clusters_brightness[3] = {NULL, NULL, NULL};
    relative_brightness_of_color(w, h, img, clusters_brightness, img1);  // Добавляем аргумент img1

    Node* clusters_color[3] = {NULL, NULL, NULL};
    for (int i = 0; i < w * h; i++) {
        if (img1[i] < 85) {
            addNode(&clusters_color[0], i);
        } else if (img1[i] < 170) {
            addNode(&clusters_color[1], i);
        } else {
            addNode(&clusters_color[2], i);
        }
    }

    gauss_smoothing(w, h, img1, img2);
    apply_color_pattern(w, h, img2, img4, clusters_color);

    char *new_image = "skull-modified.png";
    writePng(new_image, img4, w, h);

    return 0;
}
