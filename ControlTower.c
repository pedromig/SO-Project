#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

#define LINE_BUF 15

config_t load_struct(const int *array) {
    config_t config;

    config.time_units = array[0];
    config.takeoff_time = array[1];
    config.takeoff_gap = array[2];
    config.landing_time = array[3];
    config.landing_gap = array[4];
    config.holding_min = array[5];
    config.holding_max = array[6];
    config.max_departures = array[7];
    config.max_arrivals = array[8];

    return config;
}

config_t read_config(char *fname) {
    int i = 0;
    char line[LINE_BUF], *token = NULL, delimiter[3] = ", ";
    int array[9];

    FILE *fp = fopen(fname, "r");

    if (!fp) {
        perror("Failed to open configuration file!\n");
        exit(0);
    }

    while (fgets(line, LINE_BUF, fp) != NULL) {
        strtok(line, delimiter);
        token = strtok(NULL, delimiter);
        array[i++] = atoi(line);
        if (token != NULL)
            array[i++] = atoi(token);
    }

    if (i > 9) {
        printf("Error reading file!\n");
        exit(0);
    }
    return load_struct(array);
}

config_t read_config(char *fname) {
    int i = 0;
    int array[9];
    char line[LINE_BUF], token = NULL, delimiter[2] = ", ";
   
    FILE *fp = fopen(fname, "r");

    if (!fp) {
        perror("Failed to open configuration file!\n");
        exit(0);
    }

    while (fgets(line, LINE_BUF, fp) != NULL) {
        token = strtok(line, delimiter);
        while (token != NULL) {
            if (i > 9) {
                printf("Error reading file!\n");
                exit(0);
            }
            array[i++] = atoi(token);
            strtok(NULL, delimiter);
        }
    }

    return load_struct(array);
}
