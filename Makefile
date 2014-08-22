CC := gcc-4.7

CFLAGS := -D_GNU_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE
CFLAGS += -DDEBUG -DNDEBUG -O0
CFLAGS += -g3
CFLAGS += -ggdb
CFLAGS += -fvar-tracking-assignments
CFLAGS += -Wstrict-aliasing=3 -Wstrict-overflow=5 -Wmissing-field-initializers \
	  -fno-omit-frame-pointer -fstack-protector-all -Wmissing-declarations \
	  -Wdouble-promotion -Wwrite-strings -funit-at-a-time -Wnormalized=nfc \
	  -Wcast-qual -Wclobbered -Wempty-body -Wvector-operation-performance  \
	  -fstrict-overflow -Wmissing-format-attribute -Wpacked -Wfloat-equal  \
	  -Wunsuffixed-float-constants -Wjump-misses-init -Waggregate-return   \
	  -Wold-style-declaration -Wmissing-parameter-type -Wnested-externs    \
	  -Wbad-function-cast -Wdeclaration-after-statement -Woverride-init    \
	  -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition      \
	  -Wignored-qualifiers -Winline -ftree-vrp -Wformat=2 -Winit-self      \
	  -Wunused-but-set-parameter -Wredundant-decls -Wstack-protector       \
	  -Wlogical-op -Wtrampolines -Wsuggest-attribute=const -Wunused        \
	  -Wswitch-default -fstrict-aliasing -Wcast-align -Wconversion         \
	  -march=native -Wsign-compare -Wuninitialized -Winvalid-pch           \
	  -Wmissing-include-dirs -Wdisabled-optimization -Wsync-nand           \
	  -Wall -Wvla -ftrapv -Wundef -Wshadow -Woverlength-strings            \
	  -Wunsafe-loop-optimizations -funsafe-loop-optimizations              \
	  -D_FORTIFY_SOURCE=2 -Wswitch-enum -Wtype-limits -Wextra

CFLAGS += -std=gnu89 -Werror #-pedantic

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
	cp scripts/*.sh /tmp

clean:
	rm -f $(APP) *.o core
