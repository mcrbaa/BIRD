#include "QRCodeScanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <zbar.h>
#include <jpeglib.h>

/* Load a JPEG file into grayscale image buffer */
unsigned char* load_jpeg(const char *filename, int *width, int *height) {
    FILE *file = fopen(filename, "rb"); //Open JPEG file in binary read mode
    if (!file) {
        fprintf(stderr, "Error: Could not open JPEG file %s\n", filename);
        return NULL;    //Return NULL if file cannot be opened
    }

    /* Initialize the JPEG decompression structure & error handler */
    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;
    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);
    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);
    jpeg_start_decompress(&info);


    /* Set image width, height, and row stride (number of bytes per row) */
    *width = info.output_width;
    *height = info.output_height;
    int row_stride = *width * info.output_components;

    /* Allocate memory for the imagge data buffer */
    unsigned char *image_data = (unsigned char*)malloc(row_stride * (*height));
    if (!image_data) {
        fprintf(stderr, "Error: Could not allocate memory for image\n");
        fclose(file);
        jpeg_destroy_decompress(&info);
        return NULL;
    }


    /* Read image data row by row into the image_data buffer */
    unsigned char *row_pointer = image_data;

    while (info.output_scanline < *height) {
        jpeg_read_scanlines(&info, &row_pointer, 1);    // One row read at a time
        row_pointer += row_stride;  //Next row
    }

    /* Finish decompression & cleanup */
    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
    fclose(file);

    return image_data;  //Return image data buffer
}

// Convert RGB image to grayscale
void convert_to_grayscale(unsigned char *image_data, int width, int height) {
    int row_stride = width * 3;  // RGB input (3 bytes per pixel)
    unsigned char *grayscale_data = (unsigned char*)malloc(width * height);  // single channel for grayscale

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            /* Extract RGB values */
            unsigned char r = image_data[(y * row_stride) + (x * 3) + 0];
            unsigned char g = image_data[(y * row_stride) + (x * 3) + 1];
            unsigned char b = image_data[(y * row_stride) + (x * 3) + 2];
            /* Calculate grayscale value and store it */
            unsigned char gray = (r + g + b) / 3;
            grayscale_data[(y * width) + x] = gray;  // Store grayscale value
        }
    }

    // Copy grayscale data to the original & free temporary memory
    memcpy(image_data, grayscale_data, width * height);
    free(grayscale_data);
}

// Function to scan QR code using ZBar
int scan_qrcode(const char *filename) {
    int width, height;
    unsigned char *image_data = load_jpeg(filename, &width, &height);
    if (!image_data) {
        return -1;  //Exit if the image couldn't be loaded
    }

    printf("Image loaded. Width: %d, Height: %d\n", width, height);

    // Convert to grayscale
    convert_to_grayscale(image_data, width, height);
    printf("Image converted to grayscale\n");

    // ZBar scanner setup
    zbar_image_scanner_t *scanner = zbar_image_scanner_create();
    if (!scanner) {
        fprintf(stderr, "Error: Could not create ZBar scanner\n");
        free(image_data);  // Free the image data if scanner creation failed
        return -1;
    }

    /*Configure QR code scanning */
    zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);

    // Create ZBar image
    zbar_image_t *image = zbar_image_create();
    if (!image) {
        fprintf(stderr, "Error: Could not create ZBar image\n");
        zbar_image_scanner_destroy(scanner);
        free(image_data);  // Free image data if image creation fails
        return -1;
    }

    /* Zbar Image properties */
    zbar_image_set_format(image, *(int*)"Y800");  // Set grayscale format (Y800)
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, image_data, width * height, zbar_image_free_data);

    /* Scan image */
    int result = zbar_scan_image(scanner, image);

    if (result > 0) {
        const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
        int found = 0;
        for (; symbol; symbol = zbar_symbol_next(symbol)) {
            if (zbar_symbol_get_type(symbol) == ZBAR_QRCODE && !found) {
                printf("QR Code data: %s\n", zbar_symbol_get_data(symbol));
                found = 1;  // Stop after the first QR code is found
            }
        }
    } else {
        printf("No QR code found.\n");
    }

    /* Cleanup by destroying image & scanner */
    if (image) {
        zbar_image_destroy(image); 
    }
    if (scanner) {
        zbar_image_scanner_destroy(scanner);
    }

    return result > 0 ? 0 : -1; //Return success if QRcode found, failure if anything else
}  
