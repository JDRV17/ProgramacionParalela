#include <iostream>
#include <cstring>
#include <ctime>
#include <chrono>
#include "image.h"
#include "pgmimage.h"
#include "ppmimage.h"

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {

  if(argc<4){
    std::cout << "Missing input and output paths\n";
    std::cout << "Usage:" << argv[0] << " input_image.pgm output_image.pgm --f [blur|laplace|sharpening]" << std::endl;
    std::cout << "or "<< argv[0] << "input_image.ppm output_image.ppm --f [blur|laplace|sharpening]" << std::endl;
    return 1;
  }

  auto wall_start = std::chrono::high_resolution_clock::now();

  const char* filterType = nullptr;
  for (int i = 3; i < argc; i++) {
      if (strcmp(argv[i], "--f") == 0 && i + 1 < argc) {
          filterType = argv[i + 1];
          break;
      }
  }
  if (filterType == NULL) {
    std::cout << "Error, must specify a filter with --f" << std::endl;
    return 1;
  }
  if (strcmp(filterType, "blur") != 0 && 
    strcmp(filterType, "laplace") != 0 && 
    strcmp(filterType, "sharpening") != 0) {
    std::cout << "Error, wrong filter. Use blur, laplace or sharpening" << std::endl;
    return 1;
  }

  // abrir archivo
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

  clock_t cpu_start = clock();
  image->applyFilter(filterType);
  clock_t cpu_end = clock();
  double cpu_time = double(cpu_end - cpu_start) / CLOCKS_PER_SEC;

  FILE *output = fopen(argv[2], "w");
  if (output == NULL) {
    std::cout << "Error, could not create the output file."<< std::endl;
    delete image;
    return 1;
  }

  image->save(output);
  fclose(output);

  auto wall_end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> wall_time = wall_end - wall_start;

  delete image;

  std::cout << "CPU Time (applying the filter only): " << cpu_time << " seconds" << std::endl;
  std::cout << "Total Execution Time: " << wall_time.count() << " seconds" << std::endl;
  
  return 0;
}