CC=gcc
OPT=-O3
CFLAGS= -std=gnu99 -g -fPIC -DPIC -shared -rdynamic -Wall -fstack-protector-all
BUILD=./build
SRC=./src

build: creatbuild pam_encfs.so

testing: creatbuild pam_encfs_test.so

creatbuild:
	mkdir -p $(BUILD)

mount_encfs.o:
	$(CC) $(OPT) $(CFLAGS) -c $(SRC)/mount_encfs.c -o $(BUILD)/mount_encfs.o

utils.o:
	$(CC) $(OPT) $(CFLAGS) -c $(SRC)/utils.c -o $(BUILD)/utils.o

config.o: 
	$(CC) $(OPT) $(CFLAGS) -c $(SRC)/config.c -o $(BUILD)/config.o

pam_encfs.o:
	$(CC) $(OPT) $(CFLAGS) -c $(SRC)/pam_encfs.c -o $(BUILD)/pam_encfs.o

pam_encfs_test.o:
	$(CC) $(OPT) $(CFLAGS) -c $(SRC)/pam_encfs_test.c -o $(BUILD)/pam_encfs_test.o

pam_encfs.so: mount_encfs.o utils.o config.o pam_encfs.o
	$(CC) $(OPT) $(CFLAGS) $(BUILD)/config.o \
	    $(BUILD)/utils.o $(BUILD)/mount_encfs.o \
	    $(BUILD)/pam_encfs.o -o $(BUILD)/pam_encfs.so

pam_encfs_test.so: mount_encfs.o utils.o config.o pam_encfs_test.o
	$(CC) $(OPT) $(CFLAGS) $(BUILD)/config.o \
	    $(BUILD)/utils.o $(BUILD)/mount_encfs.o \
	    $(BUILD)/pam_encfs_test.o -o $(BUILD)/pam_encfs.so

clean:
	rm -rf $(BUILD)/*.so
	rm -rf $(BUILD)/*.o
