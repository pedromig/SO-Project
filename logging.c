#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "logging.h"

/**
 * This function gets the current time of the system
 * and formats the time placing it in a string passed
 * by parameter
 * @param time_str String to be loaded with the system time
 *                 properly formated.The String must be allocated
 *                 with the size (9 bytes) adjusted to the format HH:MM:SS
 *
 * @return status 0 if success getting the time
 *               -1 if error getting the time
 */

int sys_time(char *time_str) {
    int str_size = 9, status = 0;
    time_t t;
    struct tm *t_info;

    t = time(NULL);
    if (t == (time_t) -1) {
        status = -1;
    } else {
        t_info = localtime(&t);
        strftime(time_str, str_size, "%T", t_info);
    }
    return status;
}

/**
 * This function cleans the log file 
 * that is opened in append mode using the
 * function open_log()
 * @param void
 * @return void
 * 
 */

void clean_log() {
    FILE *fp = fopen(LOG_PATH, "w");
    printf("Cleaning log file... ");
    fclose(fp);
    printf("Done!\n");
}

/**
 * This function opens a log file in
 * append mode.
 * @param log The log file name or path
 * @return fp A file pointer to the log file
 *            we opened 
 */

FILE *open_log(char *log) {

    FILE *fp = fopen(log, "a");
    if (!fp) {
        printf("Error opening log file!\n");
        exit(0);
    }

    return fp;
}

/**
 * This function logs all the arrival landings
 * @param fp File pointer to the output stream
 * @param flight The flight we want to log
 * @param runway The runway associated with the flight
 * @param state CONCLUDED(1) or STARTED(0)
 * @return void
 */

void log_landing(FILE *fp, char *flight, char *runway, int state) {
    char time[TIME_SIZE];

    sys_time(time);
    if (state == STARTED) {
        fprintf(fp, "%s %s %s %s %s \n", time, flight, "LANDING", runway, "started");
    } else if (state == CONCLUDED) {
        fprintf(fp, "%s %s %s %s %s \n", time, flight, "LANDING", runway, "concluded");
    } else {
        fprintf(fp, "Invalid log!...\n");
    }

}

/**
 * This function logs all the departure landings
 * @param fp File pointer to the output stream
 * @param flight The flight we want to log
 * @param runway The runway associated with the flight
 * @param state CONCLUDED(0) or STARTED(1)
 * @return void
 */

void log_departure(FILE *fp, char *flight, char *runway, int state) {
    char time[TIME_SIZE];

    sys_time(time);
    if (state == STARTED) {
        fprintf(fp, "%s %s %s %s %s\n", time, flight, "DEPARTURE", runway, "started");
    } else if (state == CONCLUDED) {
        fprintf(fp, "%s %s %s %s %s\n", time, flight, "DEPARTURE", runway, "concluded");
    } else {
        fprintf(fp, "Invalid log!...\n");
    }
}

/**
 * This function logs the holdings of the flights
 * @param fp File pointer to the output stream
 * @param flight The flight we want to log
 * @param holding_time Time the flight will be put on hold
 *                     waiting to land
 * @return void
 */

void log_holding(FILE *fp, char *flight, int holding_time) {
    char time[TIME_SIZE];

    sys_time(time);
    fprintf(fp, "%s %s %s %d\n", time, flight, "HOLDING", holding_time);

}

/**
 * This function logs the command the user
 * has inserted through the pipe
 * @param fp File pointer to the output stream
 * @param cmd The command the user entered
 * @param status NEW_COMMAND(0) WRONG_COMMAMD(1)
 * @return void
 */

void log_command(FILE *fp, char *cmd, int status) {
    char time[TIME_SIZE];

    sys_time(time);
    if (status == NEW_COMMAND) {
        fprintf(fp, "%s %s => %s\n", time, "NEW COMMAND", cmd);
    } else if (status == WRONG_COMMAMD) {
        fprintf(fp, "%s %s => %s\n", time, "WRONG COMMAND", cmd);
    } else {
        fprintf(fp, "Invalid log!...\n");
    }

}

/**
 * This fuction logs a emergency flight
 * @param fp File pointer to the output stream
 * @param flight The flight we want to log
 * @return void
 */

void log_emergency(FILE *fp, char *flight) {
    char time[TIME_SIZE];

    sys_time(time);
    fprintf(fp, "%s %s %s\n", time, flight, "EMERGENCY LANDING REQUESTED");
}

/**
 * This function logs a detour flight
 * to the other airport.
 * TODO: Maybe missing some parameters... not sure
 * 
 * @param fp File pointer to the output stream
 * @param flight The flight we want to log
 * @param fuel The plane fuel in fuel units
 * @return void
 */

void log_detour(FILE *fp, char *flight, int fuel) {
    char time[TIME_SIZE];

    sys_time(time);
    fprintf(fp, "%s %s %s => FUEL => %d\n", time, flight, "LEAVING TO OTHER AIRPORT", fuel);

}

/**
 * This function logs the program status 
 * the current status implemented are 
 * program start and program end
 * @param fp File pointer to the output stream
 * @param program_status STARTED(1) CONCLUDED(0)
 */

void log_status(FILE *fp, int program_status) {
    char time[TIME_SIZE];

    sys_time(time);

    if (program_status == STARTED) {
        fprintf(fp, "%s %s\n", time, "<----- SIMULATION MANAGER STARTED ----->");
    } else if (program_status == CONCLUDED) {
        fprintf(fp, "%s %s\n", time, "<----- SIMULATION MANAGER ENDED ----->");
    } else {
        printf("Invalid log!...\n");
    }

}