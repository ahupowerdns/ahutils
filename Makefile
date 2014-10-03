-include sysdeps/$(shell uname).inc

VERSION=0.1
CXXFLAGS?=-Wall -O3 -MMD -MP  -std=gnu++0x  $(CXX2011FLAGS) -pthread # -Wno-unused-local-typedefs 
LDFLAGS=$(CXX2011FLAGS) -pthread

PROGRAMS=testrunner ltest

all: $(PROGRAMS)

clean:
	-rm  -f *.d *.o *~ testrunner

testrunner: rfile.o testrunner.o
	$(CXX) $(CXXFLAGS) $^ -o $@

ltest: lmdbwrap.o ltest.o 
	$(CXX) $(CXXFLAGS) $^ -llmdb -o $@


-include *.d
