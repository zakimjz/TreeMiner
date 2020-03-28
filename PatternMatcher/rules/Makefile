CC = g++
CFLAGS = -O3
HDRS = countfreq.h timetrack.h calcdb.h idlist.h eqclass.h freqit.h
OBJS = calcdb.o idlist.o countfreq.o eqclass.o freqit.o
LIBS = 
TARGET = countfreq cpairs

default: $(TARGET)

clean:
	rm -rf *~ *.o $(TARGET)

countfreq: $(OBJS) $(HFILES)
	$(CC) $(CFLAGS) -o countfreq $(OBJS) $(LIBS)
#	strip countfreq

cpairs: compare_pairs.o
	$(CC) $(CFLAGS) -o cpairs compare_pairs.o $(LIBS)


.SUFFIXES: .o .cpp

.cpp.o:
	$(CC) $(CFLAGS) -c $<


# dependencies
# $(OBJS): $(HFILES)
countfreq.o: $(HDRS)
calcdb.o: calcdb.h countfreq.h idlist.h
idlist.o: idlist.h
eqclass.o: eqclass.h
freqit.o: freqit.h
