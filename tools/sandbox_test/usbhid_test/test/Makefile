all : hidtest

ifeq ($(shell uname), Linux)
CFLAGS:=-g -O0
LDFLAGS:=-ludev
CC:=gcc
else
CFLAGS:=-Os -s
CC:=gcc
LDFLAGS:=C:/windows/system32/setupapi.dll
endif

hidtest : hidtest.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf hidtest

