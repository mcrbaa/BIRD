#include <stdio.h>
#include <stdlib.h>

int main() {
    // Call the Python script
    printf("Running Python script...\n");
    int result = system("python3 keyboard.py");

    if(result == 0) {
        printf("Python script executed successfully.\n");
    } else {
        fprintf(stderr, "Python script execution failed.\n");
    }

    return 0;
}
