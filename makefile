CC = gcc
OBJS = logging.o ControlTower.o SimulationManager.o
PROG = airport
FLAGS = -Wall -O -g -pthread
MATH = -lm

################## GENERIC #################

all: ${PROG}

clean:
	rm ${OBJS} ${PROG}

${PROG}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
	${CC} ${FLAGS} $< -c -o $@

################ DEPENDECIES ##############

ControlTower.o: ControlTower.c structs.h

SimulationManager.o: SimulationManager.c logging.o structs.h logging.h

logging.o: logging.c logging.h


