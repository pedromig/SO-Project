#ifndef LOGGING_H
#define LOGGING_H


#define LOG_PATH "log.txt"
#define TIME_SIZE 9
#define CONCLUDED 0
#define STARTED 1
#define NEW_COMMAND 0
#define WRONG_COMMAMD 1

FILE *open_log(char *log);

void clean_log(void);

void log_landing(FILE *fp, char *flight, char *runway, int state);
void log_departure(FILE *fp, char *flight, char *runway, int state);
void log_holding(FILE *fp, char *flight, int holding_time);
void log_command(FILE *fp, char *cmd, int status);
void log_emergency(FILE *fp, char *flight);
void log_detour(FILE *fp, char *flight, int fuel);
void log_status(FILE *fp, int program_status);

#endif // LOGGING_H