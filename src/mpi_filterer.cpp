#include <iostream>
#include <cstring>
#include <ctime>
#include <chrono>
#include <mpi.h>
#include "image.h"
#include "pgmimage.h"
#include "ppmimage.h"

#define TAG_WORK 1
#define TAG_RESULT 2

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if(size<4){
    if(rank == 0)
    {
      std::cout << "There needs to be 4 processes." << std::endl;
    }
    
    std::cout << "Usage:" << argv[0] << " input_image.pgm output_blur.pgm output_laplace.pgm output_sharpening.pgm" << std::endl;
    std::cout << "or "<< argv[0] << "input_image.ppm output_blur.ppm output_laplace.ppm output_sharpening.ppm" << std::endl;
    MPI_Finalize();
    return 1;
  }

  
  double start_time = MPI_Wtime();
  double cpu_start = clock();

  if (rank == 0) {
    auto wall_start = std::chrono::high_resolution_clock::now();

    Image* image = Image::createFromFile(argv[1]);
    if (image == NULL) {
      std::cout << "Error, incorrect path or incorrect file." << std::endl;
      MPI_Finalize();
      return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      std::cout << "Error, could not open the input file."<< std::endl;
      delete image;
      MPI_Finalize();
      return 1;
    }

    image->load(file);
    fclose(file);

    int width = image->getWidth();
    int height = image->getHeight();
    int maxColor = image->getMaxColor();
    const char* magic = image->getMagicNumber();

    for (int i = 1; i < 4; i++) {
        MPI_Send(&width, 1, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
        MPI_Send(&height, 1, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
        MPI_Send(&maxColor, 1, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
        
        int magic_len = strlen(magic) + 1;
        MPI_Send(&magic_len, 1, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
        MPI_Send(magic, magic_len, MPI_CHAR, i, TAG_WORK, MPI_COMM_WORLD);
    }

    if (strcmp(magic, "P2") == 0) {
        PGMImage* pgm = dynamic_cast<PGMImage*>(image);
        int* pixels = pgm->getPixels();
        int pixelCount = width * height;
        
        for (int i = 1; i < 4; i++) {
            MPI_Send(pixels, pixelCount, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
        }
    } else if (strcmp(magic, "P3") == 0) {
        PPMImage* ppm = dynamic_cast<PPMImage*>(image);
        int* pixels = ppm->getPixels();
        int pixelCount = width * height * 3;
        
        for (int i = 1; i < 4; i++) {
            MPI_Send(pixels, pixelCount, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
        }
    }

    Image* results[3];
    for (int i = 1; i < 4; i++) {
        MPI_Status status;
        int result_size;
        MPI_Recv(&result_size, 1, MPI_INT, i, TAG_RESULT, MPI_COMM_WORLD, &status);
        
        int* result_pixels = new int[result_size];
        MPI_Recv(result_pixels, result_size, MPI_INT, i, TAG_RESULT, MPI_COMM_WORLD, &status);
 
        if (strcmp(magic, "P2") == 0) {
            PGMImage* result_img = new PGMImage();
            result_img->loadFromData(magic, width, height, maxColor, result_pixels);
            results[i-1] = result_img;
        } else {
            PPMImage* result_img = new PPMImage();
            result_img->loadFromData(magic, width, height, maxColor, result_pixels);
            results[i-1] = result_img;
        }
        
        delete[] result_pixels;
    }

    const char* output_files[] = {argv[2], argv[3], argv[4]};
    for (int i = 0; i < 3; i++) {
        FILE* output = fopen(output_files[i], "w");
        if (output) {
            results[i]->save(output);
            fclose(output);
        }
        delete results[i];
    }
    
    delete image;
    
    double end_time = MPI_Wtime();

    double cpu_time = double(clock() - cpu_start) / CLOCKS_PER_SEC;
    auto wall_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> wall_time = wall_end - wall_start;

    std::cout << "MPI Total Time: " << end_time - start_time << " seconds" << std::endl;
    std::cout << "CPU Time (applying the filter only): " << cpu_time << " seconds" << std::endl;
    std::cout << "Total Execution Time: " << wall_time.count() << " seconds" << std::endl;
  

  }
  else{
    int width, height, maxColor, magic_len;
    MPI_Status status;
    
    MPI_Recv(&width, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &status);
    MPI_Recv(&height, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &status);
    MPI_Recv(&maxColor, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &status);
    MPI_Recv(&magic_len, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &status);
    
    char* magic = new char[magic_len];
    MPI_Recv(magic, magic_len, MPI_CHAR, 0, TAG_WORK, MPI_COMM_WORLD, &status);
    
    int pixelCount;
    if (strcmp(magic, "P2") == 0) {
        pixelCount = width * height;
    } else {
        pixelCount = width * height * 3;
    }
    
    int* pixels = new int[pixelCount];
    MPI_Recv(pixels, pixelCount, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD, &status);

    Image* image;
    if (strcmp(magic, "P2") == 0) {
        image = new PGMImage();
    } else {
        image = new PPMImage();
    }
    
    image->loadFromData(magic, width, height, maxColor, pixels);
    
    if (rank == 1) image->applyFilter("blur");
    else if (rank == 2) image->applyFilter("laplace");
    else if (rank == 3) image->applyFilter("sharpening");
    
    int* result_pixels;
    if (strcmp(magic, "P2") == 0) {
        result_pixels = dynamic_cast<PGMImage*>(image)->getPixels();
    } else {
        result_pixels = dynamic_cast<PPMImage*>(image)->getPixels();
    }
    
    MPI_Send(&pixelCount, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
    MPI_Send(result_pixels, pixelCount, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
    
    delete[] magic;
    delete[] pixels;
    delete image;
  }

  MPI_Finalize();

  return 0;
}