INC_DIR = include
LIB_DIR = lib

CCFLAGS = -O0 -g -I${INC_DIR}  # 0 optimizations, generate debug symbols
LDFLAGS = -lpthread -no-pie

BIN_NAME = l8
BIN_DIR = /usr/local/bin

SERVICE_NAME = l8.service
SERVICE_DIR = /etc/systemd/system/

# config file will not be overwritten if already present
CONFIG_NAME = l8.conf
CONFIG_DIR  = /etc/l8/
CONFIG_PATH = ${CONFIG_DIR}${CONFIG_NAME}


l8: run.o mutex.o config.o buffer.o connect.o
	gcc $^ -o $@ ${LDFLAGS}

run.o: run.c
# run.o: run.c config.h mutex.h buffer.h connect.h   	NOT BEING USED BY COMMAND BELOW...
	gcc -c $^ ${CCFLAGS} -o $@

%.o:  ${LIB_DIR}/%.c  ${INC_DIR}/%.h
	gcc -c $< ${CCFLAGS} -o $@

# install: install_bin install_service clean
install: install_bin ${CONFIG_PATH}

# 700 does not find the bin in usr local bin
install_bin:
	@ sudo chmod 755 ${BIN_NAME}
	@ sudo cp ${BIN_NAME} ${BIN_DIR} && echo '${BIN_NAME} installed to ${BIN_DIR}'

install_service: ${CONFIG_PATH}
	@-sudo systemctl stop ${SERVICE_NAME}   # - ignores errors
	@ sudo chmod 644 ${SERVICE_NAME}
	@ sudo cp ${SERVICE_NAME} ${SERVICE_DIR} && echo '${SERVICE_NAME} installed to ${SERVICE_DIR}'

${CONFIG_DIR}:
	@ sudo mkdir ${CONFIG_DIR}

${CONFIG_PATH}: ${CONFIG_DIR}
	@ sudo cp ${CONFIG_NAME} ${CONFIG_DIR} && echo '${CONFIG_NAME} installed to ${CONFIG_DIR}' || echo '${CONFIG_NAME} already present'
	@ sudo chmod 644 ${CONFIG_PATH}

clean:
	rm *.o ${BIN_NAME} &> /dev/null
