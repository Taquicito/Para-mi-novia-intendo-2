TARGET = novia
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspaudio -lpspdisplay -lpspctrl -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Quieres ser mi novia

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
