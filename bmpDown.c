#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <dirent.h>
#include <iostream>
#include <Windows.h>
#include <sys/stat.h>

#pragma pack(push, 1)
typedef struct {
    char signature[2];
    unsigned int file_size;
    unsigned int reserved;
    unsigned int data_offset;
    unsigned int header_size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bpp;
    unsigned int compression;
    unsigned int image_size;
    int x_ppm;
    int y_ppm;
    unsigned int colors_used;
    unsigned int important_colors;
} BMPHeader;
#pragma pack(pop)

int batch, defaults = 0;
const char *name = "bmpDown";

// Function to read BMP file header
void readBMPHeader(FILE* file, BMPHeader* header) {
    fread(header, sizeof(BMPHeader), 1, file);
}

// Function to write BMP file header
void writeBMPHeader(FILE* file, BMPHeader* header) {
    fwrite(header, sizeof(BMPHeader), 1, file);
}



// Function to get the color value of a pixel at coordinates (x, y)
void getPixelColor(const unsigned char* data, int width, int x, int y, unsigned char* color) {
    int offset = (y * width + x) * 3;
    for (int i = 0; i < 3; i++) {
        color[i] = data[offset + i];
        //color[i] = data[i];
    }
}




// Function to downsize the BMP image by a scale factor using bilinear interpolation
void downsizeBMP(char* input_file_name, char* output_file_name, float scale) {
    FILE* input_file = malloc(sizeof(FILE));
    FILE *output_file = malloc(sizeof(FILE));
    fopen_s(input_file, input_file_name, "rb");
    fopen_s(output_file, output_file_name, "wb");
    if (input_file == NULL || output_file == NULL) {
        printf("Error opening files: [%s] and [%s].\n", input_file_name, output_file_name);
        return;
    }
    BMPHeader header;
    readBMPHeader(input_file, &header);
    //printf("bmpheader.size: %d\n", header.file_size);
    // Calculate the new width and height after downsizing
    
    int new_width = (int)(header.width * scale);
    int new_height = (int)(header.height * scale);

    // Calculate padding for the original and new image
    int original_padding = (4 - (header.width * 3) % 4) % 4;
    int new_padding = (4 - (new_width * 3) % 4) % 4;

    // Calculate the new image size and file size
    int new_image_size = (new_width * 3 + new_padding) * new_height;
    int new_file_size = new_image_size + sizeof(BMPHeader);

    // Update header with the new image size and dimensions
    int fsize, width, height, size;
    fsize = header.file_size;
    width = header.width;
    height = header.height;
    size = header.image_size;
    header.file_size = new_file_size;
    header.width = new_width;
    header.height = new_height;
    header.image_size = new_image_size;
    //printf("bmpheader.size: %d\n", header.file_size);

    // Write the new header to the output file
    writeBMPHeader(output_file, &header);

    // Allocate memory for reading the original image data
    unsigned char* original_data = (unsigned char*)malloc(size);

    // Read the original image data
    fseek(input_file, sizeof(BMPHeader), SEEK_SET);
    fread(original_data, size, 1, input_file);

    // Allocate memory for the downsized image data
    unsigned char* downsized_data = (unsigned char*)malloc(new_image_size);

    // Downsize the image data using bilinear interpolation
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            float orig_x = x / scale;
            float orig_y = y / scale;

            int x1 = (int)orig_x;
            int y1 = (int)orig_y;
            int x2 = x1 + 1;
            int y2 = y1 + 1;

            unsigned char color1[3], color2[3], color3[3], color4[3];

            getPixelColor(original_data, width, x1, y1, color1);
            getPixelColor(original_data, width, x2, y1, color2);
            getPixelColor(original_data, width, x1, y2, color3);
            getPixelColor(original_data, width, x2, y2, color4);

            int new_offset = (y * new_width + x) * 3;
            for (int i = 0; i < 3; i++) {
                float dx = orig_x - x1;
                float dy = orig_y - y1;
                downsized_data[new_offset + i] = (unsigned char)(
                    color1[i] * (1 - dx) * (1 - dy) +
                    color2[i] * dx * (1 - dy) +
                    color3[i] * (1 - dx) * dy +
                    color4[i] * dx * dy
                );
            }
        }
    }

    // Write the downsized image data to the output file
    fwrite(downsized_data, sizeof(unsigned char), new_image_size, output_file);
    printf("Image downsized and saved to %s\n", output_file_name);
    // Free the allocated memory
    free(original_data);
    free(downsized_data);
    fclose(input_file);
    fclose(output_file);
}




void batchRun(char *inputBatchDir, char *outputBatchDir, float scale){
    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFile("C:\\semester2", &data);

    char filename_qfd[255 * 8];
    char combinedFileName[256 * 8];
    char combinedOutputFileName[256 * 8];

    if (hFind != INVALID_HANDLE_VALUE) {
        while (FindNextFile(hFind, &data)) {
            if (strstr(data.cFileName, ".bmp") != NULL) {
                sprintf_s(filename_qfd, "%s/%s", inputBatchDir, data.cFileName);

                sprintf_s(combinedFileName, "%s/%s", inputBatchDir, data.cFileName);
                sprintf_s(combinedOutputFileName, "%s/%s", outputBatchDir, data.cFileName);
                downsizeBMP(combinedFileName, combinedOutputFileName, scale);
            }
            
        }
    }
    else {
        fprintf(stderr, "Can't open %s\n", inputBatchDir);
        return;
    }
}

int parseArgs(int argc, char *argv[], char *args[]){
    int done = 0;
    int i = 1;
    args[0] = argv[0];
    while(i<argc && !done) { // -t target Type; -o output file -i input files
        char *d; // dash
        d=argv[i];
        if (d[0] == '-'  )
        {
        ++d; //strips off dash
        switch(d[0]) {
            case 'I':
            case 'i': // inputFile
                i++;
                args[1] = argv[i];
                break;

            case 'O':
            case 'o': // outputFile
                i++;
                args[2] = argv[i];
                break;

            case 'S':
            case 's': // transformScale (scale out of 1.0)
                i++;
                args[3] = argv[i];
                break;

            case 'B':
            case 'b': // whether or not it's a batch of files
                batch = 1;
                defaults = 1;
                break;

            case 'D':
            case 'd': // whether or not to use defualt filenames (idk if i need this yet)
                defaults = 1;
                break;
            
            }
        }
        i++;
    }
}

int main(int argc, char *argv[]) {
    if (argc!=8 && argc!=7){
        printf("Usage: %s -i inputFile/dir -o outputFile/dir -s scale -b [batchModeFlag]\n", name);
        return -1;
    }
    //const char* input_file_name = "icon_chartlibrary_save.bmp";
    char *args[8];
    parseArgs(argc, argv, args);
    // char* input_file_name = argv[1];
    // char* output_file_name = argv[2];
    // const float new_size = atof(argv[3]); 
    
    // Change this value to adjust the downsizing scale

    //float temp = atol(argv[3]);
    //long temp = (long) argv[3];
    
    printf("input: %s output: %s scale: %f batch: %d\n", args[1], args[2], atof(args[3]), batch);
    if (!batch){
        downsizeBMP(args[1], args[2], atof(args[3]));
    }
    else{
        batchRun(args[1], args[2], atof(args[3]));
    }
    // const char* input_file_name = argv[1];
    // const char* output_file_name = argv[2];
    // const float new_size = atoi(argv[3]); // Change this value to adjust the downsizing scale

    

    

    //downsizeBMP(input_file_name, output_file_name, new_size);

    

    
    return 0;
}
