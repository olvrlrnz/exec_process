CC := cc

CFLAGS := -std=gnu89
CFLAGS += -D_GNU_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE
CFLAGS += -DDEBUG -DNDEBUG -O0 -g3 -ggdb
CFLAGS += -W -Wall -Wextra -Werror

PREFIX := $(shell pwd)
CFLAGS += -DPREFIX=\"$(PREFIX)\"

APP := proc_exec

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

%.o: %.c
	echo "[CC] $<"
	$(CC) $(CFLAGS) -c -o $@ $<

.SILENT:

$(APP): $(OBJ)
	echo "[LD] $(APP)"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(APP) *.o core
