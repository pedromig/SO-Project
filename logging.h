#ifndef LOGGING_H
#define LOGGING_H

#define LOG_PATH "log.txt"
#define TIME_SIZE 20
#define CONCLUDED 0
#define STARTED 1
#define NEW_COMMAND 0
#define WRONG_COMMAND 1
#define TERMINAL stdout
#define OFF 0
#define ON 1

#define LBLUE   "\x1B[36m"
#define YELLOW   "\x1B[33m"
#define RED   "\x1B[31m"
#define GREEN  "\x1B[32m"
#define RESET "\x1B[0m"
#define BLUE   "\x1B[34m"
#define MAG   "\x1B[35m"

FILE *open_log(char *log,int clean_log);

void log_landing(FILE *fp, char *flight, char *runway, int state, int terminal);

void log_departure(FILE *fp, char *flight, char *runway, int state, int terminal);

void log_holding(FILE *fp, char *flight, int holding_time, int terminal);

void log_command(FILE *fp, char *cmd, int status, int terminal);

void log_emergency(FILE *fp, char *flight, int terminal);

void log_detour(FILE *fp, char *flight, int fuel, int terminal);

void log_status(FILE *fp, int program_status, int terminal);

void log_error(FILE *fp, char *error, int terminal);

void log_debug(FILE *fp, char *debug_msg, int terminal);

void log_info(FILE *fp,char *info_msg, int terminal);

#endif // LOGGING_H