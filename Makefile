CC = g++-4.7
LIBRARY = linux++.so
CFLAGS = -std=gnu++11 -Wall -Wextra -Weffc++ -Wshadow -Iinclude -fPIC
#LDFLAGS = -shared
DEBUG = -ggdb3

HDRS := $(wildcard *.h)
SRCS := $(wildcard *.C)
OBJS := $(patsubst %.C,%.o,$(SRCS))
DEPS := $(patsubst %.C,%.d,$(SRCS))

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

%.o: %.C
	$(CC) $(CFLAGS) $(DEBUG) -c $< 

clean:
	-rm $(LIBRARY) *.o 2>/dev/null

again: clean all

depend: $(DEPS)

%.d: %.C
	$(CC) $(CFLAGS) -MM $< > $@
	sed -e 's/\(\w\+\)\.o/\1.d/' $@ >> $@

.PHONY: depend all

-include $(DEPS)
