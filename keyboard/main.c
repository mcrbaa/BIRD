#include <stdio.h>
#include "QRCodeScanner.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_filename>\n", argv[0]);
        return 1;
    }

    if (scan_qrcode(argv[1]) == 0) {
        printf("QR Code scanned successfully.\n");
    } else {
        printf("Failed to scan QR code.\n");
    }

    return 0;
}
