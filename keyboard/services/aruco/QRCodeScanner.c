#include "QRCodeScanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <zbar.h>
#include <jpeglib.h>

// Load a JPEG file into grayscale image buffer
unsigned char* load_jpeg(const char *filename, int *width, int *height) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open JPEG file %s\n", filename);
        return NULL;
    }

    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;
    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);
    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);
    jpeg_start_decompress(&info);

    *width = info.output_width;
    *height = info.output_height;
    int row_stride = *width * info.output_components;

    unsigned char *image_data = (unsigned char*)malloc(row_stride * (*height));
    if (!image_data) {
        fprintf(stderr, "Error: Could not allocate memory for image\n");
        fclose(file);
        jpeg_destroy_decompress(&info);
        return NULL;
    }

    unsigned char *row_pointer = image_data;

    while (info.output_scanline < *height) {
        jpeg_read_scanlines(&info, &row_pointer, 1);
        row_pointer += row_stride;
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
    fclose(file);

    return image_data;
}

// Convert image to grayscale (correctly update the format for ZBar)
void convert_to_grayscale(unsigned char *image_data, int width, int height) {
    int row_stride = width * 3;  // assuming RGB input
    unsigned char *grayscale_data = (unsigned char*)malloc(width * height);  // single channel for grayscale

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char r = image_data[(y * row_stride) + (x * 3) + 0];
            unsigned char g = image_data[(y * row_stride) + (x * 3) + 1];
            unsigned char b = image_data[(y * row_stride) + (x * 3) + 2];
            unsigned char gray = (r + g + b) / 3;
            grayscale_data[(y * width) + x] = gray;  // Store grayscale value
        }
    }

    // Copy back to the original image data
    memcpy(image_data, grayscale_data, width * height);
    free(grayscale_data);
}

// Function to scan QR code using ZBar
int scan_qrcode(const char *filename) {
    int width, height;
    unsigned char *image_data = load_jpeg(filename, &width, &height);
    if (!image_data) {
        return -1;
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

    // Enable QR code scanning
    zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);

    // Create ZBar image
    zbar_image_t *image = zbar_image_create();
    if (!image) {
        fprintf(stderr, "Error: Could not create ZBar image\n");
        zbar_image_scanner_destroy(scanner);
        free(image_data);  // Free image data if image creation fails
        return -1;
    }

    zbar_image_set_format(image, *(int*)"Y800");  // Set grayscale format (Y800)
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, image_data, width * height, zbar_image_free_data);

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

    // Ensure proper cleanup
    if (image) {
        zbar_image_destroy(image);  // ZBar will free the image data automatically
    }
    if (scanner) {
        zbar_image_scanner_destroy(scanner);
    }

    return result > 0 ? 0 : -1;
}
