#include "interf/wrapper.h"
#include "interface.c"


void stringToUint32Array(const char* inputString, uint32_t outputArray[ARRAY_SIZE]) {
    // Ensure the output array is initialized to 0
    memset(outputArray, 0, ARRAY_SIZE * sizeof(uint32_t));

    // Convert each 8-character substring to uint32_t and store in outputArray
    int i;
    int length = 8;
    for (i = 0; i < ARRAY_SIZE; ++i) {
        char substring[9]; // Buffer to hold the substring (8 characters + null terminator)
        int start = i * length; // Calculate the start index of the substring
        if (start >= strlen(inputString)) {
            // If start index is beyond the end of inputString, break the loop
            break;
        }
        strncpy(substring, inputString + start, length); // Copy the substring from inputString
        substring[length] = '\0'; // Null-terminate the substring
        char *ptr;
        outputArray[ARRAY_SIZE-1-i] = strtoul(substring, &ptr, 10); // Convert substring to uint32
    }
}



// void uint32ArrayToString(char* outputString, const uint32_t inputArray[ARRAY_SIZE2]) {
//     int i, j = 0;
//     for (i = 0; i < ARRAY_SIZE2; ++i) {
//         char buf[11]; // Buffer to hold the string representation of each uint32_t element
//         snprintf(buf, sizeof(buf), "%" PRIu32, inputArray[ARRAY_SIZE2-1-i]);
//         int len = strlen(buf);
//         memcpy(outputString + j, buf, len);
//         j += len;
//     }
//     outputString[j] = '\0';
// }

void uint32ArrayToString(char* outputString, const uint32_t inputArray[ARRAY_SIZE2]) {
    int i, j = 0;
    for (i = 0; i < ARRAY_SIZE2; ++i) {
        uint32_t element = inputArray[ARRAY_SIZE2-1-i];
        for (int k = 0; k < sizeof(uint32_t); ++k) {
            outputString[j++] = (char)(element & 0xFF);
            element >>= 8;
        }
    }
    outputString[j] = '\0';
}

void calculate_hash_c(const char* inputString, char* outputString, uint32_t number_blocks, double* elapsed_time) {
    uint32_t outputArray[ARRAY_SIZE];

    // Transform the string into an array of uint32_t
    stringToUint32Array(inputString, outputArray);
    
    uint32_t resultArray[ARRAY_SIZE2];
    // start the execution of the C - HW interface
    // timing of the interface itself
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    interface_c(outputArray, number_blocks, resultArray);
    clock_gettime(CLOCK_MONOTONIC, &end);
    *elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; //in seconds
    // convert the array of uint32_t back to a string to send to c++
    uint32ArrayToString(outputString, resultArray);

    
}