#include "pgmimage.h"
#include <cstring>
#include <algorithm>
#include <pthread.h>

const float BLUR_KERNEL[3][3] = {{1.0/9, 1.0/9, 1.0/9}, {1.0/9, 1.0/9, 1.0/9}, {1.0/9, 1.0/9, 1.0/9}};
const float LAPLACE_KERNEL[3][3] = {{0, 1, 0}, {1, -4, 1}, {0, 1, 0}};
const float SHARPEN_KERNEL[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};

PGMImage::PGMImage() : pixels(nullptr) {}
PGMImage::~PGMImage() { delete[] pixels; }

void PGMImage::load(FILE* input) {
    fseek(input, 0, SEEK_SET);
    fscanf(input, "%2s", magicNumber);
    fscanf(input, "%d %d", &width, &height);
    fscanf(input, "%d", &maxColor);
    
    int pixelCount = width * height;
    pixels = new int[pixelCount];
    
    for (int i = 0; i < pixelCount; i++) {
        fscanf(input, "%d", &pixels[i]);
    }
}

void PGMImage::save(FILE* output) {
    fprintf(output, "%s\n%d %d\n%d\n", magicNumber, width, height, maxColor);
    
    int pixelCount = width * height;
    for (int i = 0; i < pixelCount; i++) {
        fprintf(output, "%d\n", pixels[i]);
    }
}

void PGMImage::applyFilter(const char* filterType) {
    if (strcmp(filterType, "blur") == 0) applyKernel(BLUR_KERNEL);
    else if (strcmp(filterType, "laplace") == 0) applyKernel(LAPLACE_KERNEL);
    else if (strcmp(filterType, "sharpening") == 0) applyKernel(SHARPEN_KERNEL);
}

void PGMImage::applyKernel(const float kernel[3][3]) {
    int* newPixels = new int[width * height];
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int idx = ny * width + nx;
                        sum += pixels[idx] * kernel[ky+1][kx+1];
                    }
                }
            }
            
            int result = static_cast<int>(sum);
            result = std::max(0, std::min(maxColor, result));
            newPixels[y * width + x] = result;
        }
    }
    
    delete[] pixels;
    pixels = newPixels;
}

void PGMImage::applyKernelToRegion(const float kernel[3][3], int startY, int endY, int startX, int endX, pthread_mutex_t* mutex) {
    startY = std::max(0, startY);
    endY = std::min(height, endY);
    startX = std::max(0, startX);
    endX = std::min(width, endX);

    int regionHeight = endY - startY;
    int regionWidth = endX - startX;
    int* tempPixels = new int[regionHeight * regionWidth];

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            float sum = 0.0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int idx = ny * width + nx;
                        sum += pixels[idx] * kernel[ky+1][kx+1];
                    }
                }
            }
            
            int result = static_cast<int>(sum);
            result = std::max(0, std::min(maxColor, result));

            int regionY = y - startY;
            int regionX = x - startX;
            tempPixels[regionY * regionWidth + regionX] = result;
        }
    }
    pthread_mutex_lock(mutex);
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            int regionY = y - startY;
            int regionX = x - startX;
            pixels[y * width + x] = tempPixels[regionY * regionWidth + regionX];
        }
    }
    pthread_mutex_unlock(mutex);
    
    delete[] tempPixels;
}

void PGMImage::loadFromData(const char* magic, int w, int h, int maxC, int* pix) {
    strcpy(magicNumber, magic);
    width = w;
    height = h;
    maxColor = maxC;
    
    delete[] pixels;
    pixels = new int[width * height];
    memcpy(pixels, pix, width * height * sizeof(int));
}