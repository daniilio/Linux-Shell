SRC_DIR = ../src
CMD_DIR = ${SRC_DIR}/commands
IO_DIR = ${SRC_DIR}/io_helpers
MAIN_DIR = ${SRC_DIR}/main
NET_DIR = ${SRC_DIR}/server_client
VAR_DIR = ${SRC_DIR}/variables
BACK_DIR = ${SRC_DIR}/background

OBJS = builtins.o \
	   check_builtins.o \
	   commands.o \
	   io_helpers.o \
	   mysh.o \
	   chat_helpers.o \
	   client.o \
	   server_setup.o \
	   server.o \
	   socket.o \
	   variables.o \
	   background.o

HEADERS = ${CMD_DIR}/builtins.h \
		  ${CMD_DIR}/check_builtins.h \
		  ${CMD_DIR}/commands.h \
		  ${IO_DIR}/io_helpers.h \
		  ${MAIN_DIR}/mysh.h \
		  ${NET_DIR}/chat_helpers.h \
		  ${NET_DIR}/client.h \
		  ${NET_DIR}/server_setup.h \
		  ${NET_DIR}/server.h \
		  ${NET_DIR}/socket.h \
		  ${VAR_DIR}/variables.h \
		  ${BACK_DIR}/background.h

INC_DIRS = -I${CMD_DIR} \
           -I${IO_DIR} \
           -I${MAIN_DIR} \
           -I${NET_DIR} \
           -I${VAR_DIR} \
		   -I${BACK_DIR}
CFLAGS = ${INC_DIRS} -g -Wall -Wextra -Werror

COMPILE = gcc ${CFLAGS} -c $< -o $@

all: mysh

mysh: ${OBJS}
	gcc ${CFLAGS} -o $@ $^

builtins.o: ${CMD_DIR}/builtins.c ${HEADERS}
	${COMPILE}

check_builtins.o: ${CMD_DIR}/check_builtins.c ${HEADERS}
	${COMPILE}

commands.o: ${CMD_DIR}/commands.c ${HEADERS}
	${COMPILE}

io_helpers.o: ${IO_DIR}/io_helpers.c ${HEADERS}
	${COMPILE}

mysh.o: ${MAIN_DIR}/mysh.c ${HEADERS}
	${COMPILE}

chat_helpers.o: ${NET_DIR}/chat_helpers.c ${HEADERS}
	${COMPILE}

client.o: ${NET_DIR}/client.c ${HEADERS}
	${COMPILE}

server_setup.o: ${NET_DIR}/server_setup.c ${HEADERS}
	${COMPILE}

server.o: ${NET_DIR}/server.c ${HEADERS}
	${COMPILE}

socket.o: ${NET_DIR}/socket.c ${HEADERS}
	${COMPILE}

variables.o: ${VAR_DIR}/variables.c ${HEADERS}
	${COMPILE}

background.o: ${BACK_DIR}/background.c ${HEADERS}
	${COMPILE}

clean:
	rm -f *.o mysh
