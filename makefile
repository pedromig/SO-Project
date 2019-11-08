CC = gcc
OBJS = logging.o ControlTower.o SimulationManager.o SimulationUtils.o
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

SimulationManager.o: SimulationManager.c SimulationManager.h SimulationUtils.o logging.o structs.h logging.h

SimulationUtils.o: SimulationUtils.c SimulationUtils.h structs.h

logging.o: logging.c logging.h


