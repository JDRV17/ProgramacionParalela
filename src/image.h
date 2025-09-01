#ifndef IMAGE_H
#define IMAGE_H

#include <cstdio>

class Image {
    protected:
        char magicNumber[3];  
        int width;
        int height;
        int maxColor;
        
    public:
        virtual ~Image() = default;
        virtual void load(FILE* input) = 0;
        virtual void save(FILE* output) = 0;
        virtual void applyFilter(const char* filterType) = 0;
        virtual void loadFromData(const char* magic, int width, int height, int maxColor, int* pixels) = 0;
        
        static Image* createFromFile(const char* filename); 
        
        int getWidth() const { return width; }
        int getHeight() const { return height; }
        int getMaxColor() const { return maxColor; }
        const char* getMagicNumber() const { return magicNumber; }
};

#endif