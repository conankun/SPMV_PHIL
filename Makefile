OSKI_DIR = /home/jkim3165/oski
OSKIINCS_DIR = $(OSKI_DIR)/include
OSKILIBS_DIR = $(OSKI_DIR)/lib/oski

OSKILIBS_SHARED = -Wl,-rpath -Wl,$(OSKILIBS_DIR) -L$(OSKILIBS_DIR) \
			`cat $(OSKILIBS_DIR)/site-modules-shared.txt`
OSKILIBS_STATIC = -Wl,--whole-archive \
			`cat $(OSKILIBS_DIR)/site-modules-static.txt` \
			-Wl,--no-whole-archive
		

CC = /opt/intel/compilers_and_libraries_2017.0.098/linux/bin/intel64/icc
CFLAGS = -I$(OSKI_DIR)/include -O3 -g -DDO_NAME_MANGLING -DIND_TAG_CHAR="'i'" -DDEF_IND_TYPE=1 -DVAL_TAG_CHAR="'d'" -DDEF_VAL_TYPE=2 -std=c99
CLDFLAGS_SHARED = $(OSKILIBS_SHARED) -lm
CLDFLAGS_STATIC = $(OSKILIBS_STATIC) -ldl -lm

all : example1-shared example1-static

example1-shared: example1.o
	$(CC) -o $@ $(CFLAGS) example1.o $(CLDFLAGS_SHARED)

example1-static: example1.o
	$(CC) -o $@ $(CFLAGS) example1.o $(CLDFLAGS_STATIC)

.c.o:
	$(CC) -o $@ $(CFLAGS) -c $<
	
clean:
	rm -rf example1-shared example1-static example1.o core ∗˜
