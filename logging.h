#ifndef LOGGING_H
#define LOGGING_H

#define LOG_PATH "log.txt"
#define TIME_SIZE 9
#define CONCLUDED 0
#define STARTED 1
#define NEW_COMMAND 0
#define WRONG_COMMAMD 1
#define TERMINAL stdout
#define OFF 0
#define ON 1

FILE *open_log(char *log);

void clean_log(void);

void log_landing(FILE *fp, char *flight, char *runway, int state, int terminal);

void log_departure(FILE *fp, char *flight, char *runway, int state, int terminal);

void log_holding(FILE *fp, char *flight, int holding_time, int terminal);

void log_command(FILE *fp, char *cmd, int status, int terminal);

void log_emergency(FILE *fp, char *flight, int terminal);

void log_detour(FILE *fp, char *flight, int fuel, int terminal);

void log_status(FILE *fp, int program_status, int terminal);

void log_error(FILE *fp, char *error, int terminal);

void log_debug(FILE *fp, char *debug_msg, int terminal);

void log_info(FILE *fp, char *info_description, char *info_msg, int terminal);

#endif // LOGGING_H