#include <iostream>
#include <cstring>
#include <ctime>
#include <chrono>
#include <omp.h>
#include "image.h"
#include "pgmimage.h"
#include "ppmimage.h"

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {

  if(argc<5){
    std::cout << "Missing input and output paths\n";
    std::cout << "Usage:" << argv[0] << " input_image.pgm output_blur.pgm output_laplace.pgm output_sharpen.pgm" << std::endl;
    std::cout << "or "<< argv[0] << "input_image.ppm output_blur.ppm output_laplace.ppm output_sharpen.ppm" << std::endl;
    return 1;
  }

  auto wall_start = std::chrono::high_resolution_clock::now();

  Image* image = Image::createFromFile(argv[1]);
  if (image == NULL) {
      std::cout << "Error, incorrect path or incorrect file." << std::endl;
      return 1;
  }

  FILE *file = fopen(argv[1], "r");
  if (file == NULL) {
    std::cout << "Error, could not open the input file."<< std::endl;
    delete image;
    return 1;
  }

  image->load(file);
  fclose(file);

  Image* blur = Image::createFromFile(argv[1]);
  Image* laplace = Image::createFromFile(argv[1]);
  Image* sharpen = Image::createFromFile(argv[1]);

  FILE* file2 = fopen(argv[1], "r");
  blur->load(file2);
  fclose(file2);

  FILE* file3 = fopen(argv[1], "r");
  laplace->load(file3);
  fclose(file3);

  FILE* file4 = fopen(argv[1], "r");
  sharpen->load(file4);
  fclose(file4);

  clock_t cpu_start = clock();
  
  #pragma omp parallel sections
  {
    #pragma omp section
    {
      blur->applyFilter("blur");
    }
    #pragma omp section
    {
      laplace->applyFilter("laplace");
    }
    #pragma omp section
    {
      sharpen->applyFilter("sharpening");
    }
  }
  
  clock_t cpu_end = clock();
  double cpu_time = double(cpu_end - cpu_start) / CLOCKS_PER_SEC;

  FILE* output1 = fopen(argv[2], "w");
    if (output1) {
        blur->save(output1);
        fclose(output1);
    }

  FILE* output2 = fopen(argv[3], "w");
  if (output2) {
      laplace->save(output2);
      fclose(output2);
  }

  FILE* output3 = fopen(argv[4], "w");
  if (output3) {
      sharpen->save(output3);
      fclose(output3);
  }

  auto wall_end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> wall_time = wall_end - wall_start;

  delete image;
  delete blur;
  delete laplace;
  delete sharpen;

  std::cout << "CPU Time (applying the filter only): " << cpu_time << " seconds" << std::endl;
  std::cout << "Total Execution Time: " << wall_time.count() << " seconds" << std::endl;
  
  return 0;
}