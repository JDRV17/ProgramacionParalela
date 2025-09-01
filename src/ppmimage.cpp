#include "ppmimage.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <pthread.h>

const float BLUR_KERNEL[3][3] = {{1.0/9, 1.0/9, 1.0/9}, {1.0/9, 1.0/9, 1.0/9}, {1.0/9, 1.0/9, 1.0/9}};
const float LAPLACE_KERNEL[3][3] = {{0, 1, 0}, {1, -4, 1}, {0, 1, 0}};
const float SHARPEN_KERNEL[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};

PPMImage::PPMImage() : pixels(nullptr) {}
PPMImage::~PPMImage() { delete[] pixels; }

void PPMImage::load(FILE* input) {
    fseek(input, 0, SEEK_SET);
    fscanf(input, "%2s", magicNumber);
    fscanf(input, "%d %d", &width, &height);
    fscanf(input, "%d", &maxColor);
    
    int pixelCount = width * height * 3;
    pixels = new int[pixelCount];
    
    for (int i = 0; i < pixelCount; i++) {
        fscanf(input, "%d", &pixels[i]);
    }
}

void PPMImage::save(FILE* output) {
    fprintf(output, "%s\n%d %d\n%d\n", magicNumber, width, height, maxColor);
    
    int pixelCount = width * height * 3;
    for (int i = 0; i < pixelCount; i++) {
        fprintf(output, "%d\n", pixels[i]);
    }
}

void PPMImage::applyFilter(const char* filterType) {
    if (strcmp(filterType, "blur") == 0) applyKernel(BLUR_KERNEL);
    else if (strcmp(filterType, "laplace") == 0) applyKernel(LAPLACE_KERNEL);
    else if (strcmp(filterType, "sharpening") == 0) applyKernel(SHARPEN_KERNEL);
}

void PPMImage::applyKernel(const float kernel[3][3]) {
    int* newPixels = new int[width * height * 3];  
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sumR = 0.0, sumG = 0.0, sumB = 0.0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int idx = (ny * width + nx) * 3;  
                        sumR += pixels[idx] * kernel[ky+1][kx+1];
                        sumG += pixels[idx + 1] * kernel[ky+1][kx+1];
                        sumB += pixels[idx + 2] * kernel[ky+1][kx+1];
                    }
                }
            }
            
            int idx = (y * width + x) * 3;   
            newPixels[idx] = std::max(0, std::min(maxColor, static_cast<int>(sumR)));
            newPixels[idx + 1] = std::max(0, std::min(maxColor, static_cast<int>(sumG)));
            newPixels[idx + 2] = std::max(0, std::min(maxColor, static_cast<int>(sumB)));
        }
    }
    
    delete[] pixels;
    pixels = newPixels;
}

void PPMImage::applyKernelToRegion(const float kernel[3][3], int startY, int endY, int startX, int endX, pthread_mutex_t* mutex) {
    startY = std::max(0, startY);
    endY = std::min(height, endY);
    startX = std::max(0, startX);
    endX = std::min(width, endX);

    int regionHeight = endY - startY;
    int regionWidth = endX - startX;
    int* tempPixels = new int[regionHeight * regionWidth * 3];

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            float sumR = 0.0, sumG = 0.0, sumB = 0.0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int idx = (ny * width + nx) * 3;  
                        sumR += pixels[idx] * kernel[ky+1][kx+1];
                        sumG += pixels[idx + 1] * kernel[ky+1][kx+1];
                        sumB += pixels[idx + 2] * kernel[ky+1][kx+1];
                    }
                }
            }
            
            bool isLaplace = (kernel[0][0] + kernel[0][1] + kernel[0][2] +
                             kernel[1][0] + kernel[1][1] + kernel[1][2] +
                             kernel[2][0] + kernel[2][1] + kernel[2][2]) == 0;
            
            int resultR = static_cast<int>(sumR);
            int resultG = static_cast<int>(sumG);
            int resultB = static_cast<int>(sumB);
            
            if (isLaplace) {
                resultR = std::abs(resultR);
                resultG = std::abs(resultG);
                resultB = std::abs(resultB);
            }
            
            resultR = std::max(0, std::min(maxColor, resultR));
            resultG = std::max(0, std::min(maxColor, resultG));
            resultB = std::max(0, std::min(maxColor, resultB));

            int regionY = y - startY;
            int regionX = x - startX;
            int tempIdx = (regionY * regionWidth + regionX) * 3;  
            
            tempPixels[tempIdx] = resultR;
            tempPixels[tempIdx + 1] = resultG;
            tempPixels[tempIdx + 2] = resultB;
        }
    }
    pthread_mutex_lock(mutex);
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            int regionY = y - startY;
            int regionX = x - startX;
            int tempIdx = (regionY * regionWidth + regionX) * 3;
            int pixelsIdx = (y * width + x) * 3;
            
            pixels[pixelsIdx] = tempPixels[tempIdx];
            pixels[pixelsIdx + 1] = tempPixels[tempIdx + 1];
            pixels[pixelsIdx + 2] = tempPixels[tempIdx + 2];
        }
    }
    pthread_mutex_unlock(mutex);
    
    delete[] tempPixels;
}

void PPMImage::loadFromData(const char* magic, int w, int h, int maxC, int* pix) {
    strcpy(magicNumber, magic);
    width = w;
    height = h;
    maxColor = maxC;
    
    delete[] pixels;
    pixels = new int[width * height * 3];
    memcpy(pixels, pix, width * height * 3 * sizeof(int));
}