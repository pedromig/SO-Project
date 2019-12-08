/*
 *      logging.h
 *
 *      Copyright 2019 Miguel Rabuge Nº 2018293728
 *      Copyright 2019 Pedro Rodrigues Nº 2018283166
 */

#ifndef LOGGING_H
#define LOGGING_H

// Log file path
#define LOG_PATH "log.txt"

// Size of the char array that holds the time string
#define TIME_SIZE 20

// Arrival and Departure flight log function commands
#define CONCLUDED 0
#define STARTED 1

// Input parsing log function commands
#define NEW_COMMAND 0
#define WRONG_COMMAND 1

// Auxiliary defines to make the code clear
#define TERMINAL stdout
#define OFF 0
#define ON 1

// Terminal color definitions
#define LBLUE   "\x1B[36m"
#define YELLOW   "\x1B[33m"
#define RED   "\x1B[31m"
#define GREEN  "\x1B[32m"
#define RESET "\x1B[0m"
#define BLUE   "\x1B[34m"

// Log Functions

int sys_time(char *time_str);

FILE *open_log(char *log, int clean_log);

void log_landing(FILE *fp, char *flight, char *runway, int state, int terminal);

void log_departure(FILE *fp, char *flight, char *runway, int state, int terminal);

void log_holding(FILE *fp, char *flight, int holding_time, int terminal);

void log_command(FILE *fp, char *cmd, int status, int terminal);

void log_emergency(FILE *fp, char *flight, int terminal);

void log_detour(FILE *fp, char *flight, int fuel, int terminal);

void log_status(FILE *fp, int program_status, int terminal);

void log_error(FILE *fp, char *error, int terminal);

void log_debug(FILE *fp, char *debug_msg, int terminal);

void log_info(FILE *fp, char *info_msg, int terminal);

#endif // LOGGING_H