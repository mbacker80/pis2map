TARGETNAME=main

CC = gcc.exe

#CFLAGS_DEBUG = -Wall -O0 -g -Wl,--subsystem,console -DDEBUG
CFLAGS = -O2 -s

STRIP = i586-mingw32msvc-strip

UPX=upx -9


pis2map.exe: $(TARGETNAME).c
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

release: $(TARGETNAME)
	$(STRIP) $<
	$(UPX) $<