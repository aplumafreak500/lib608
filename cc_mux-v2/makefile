# Make file for cc_mux
WD = windres
CC = gcc
LD = gcc
WDFLAGS =
CFLAGS =
LDFLAGS = -mwindows
RM = /bin/rm -f

LIBS = -lcomctl32 -lversion
OBJS = cc_mux.o muxer.o bits.o cc_mux_res.o


all: cc_mux

cc_mux : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

cc_mux.o : resource.h cc_mux.h muxer.h

muxer.o : muxer.h cc_mux.h bits.h

bits.o : bits.h cc_mux.h

cc_mux_res.o : cc_mux_res.rc resource.h
	$(WD) $(WDFLAGS) cc_mux_res.rc $@

clean:
	$(RM) $(OBJS)

