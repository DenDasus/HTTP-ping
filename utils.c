#include "utils.h"
#include <limits.h>

unsigned short FastRound(double value) {
    return value + 0.5;
}

//Calculation of the average value over an array of short values
unsigned short AvgCalc(unsigned short* array, unsigned short measCount) {
    unsigned short avg = USHRT_MAX;
    unsigned short count = 0;

    for(int i = 0; i < measCount; i++) {
        if(array[i] == USHRT_MAX) {
            continue;
        } else {
            ++count;
            avg += (array[i] - avg) / count;
        }
    }
    return avg;
}