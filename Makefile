-include sysdeps/$(shell uname).inc

VERSION=0.1
CXXFLAGS?=-Wall -O3 -MMD -MP  -std=gnu++0x  $(CXX2011FLAGS) # -Wno-unused-local-typedefs 
LDFLAGS=$(CXX2011FLAGS)   

PROGRAMS=testrunner lmdbwrap

all: $(PROGRAMS)

clean:
	-rm  -f *.d *.o *~ testrunner

testrunner: rfile.o testrunner.o
	$(CXX) $(CXXFLAGS) $^ -o $@

lmdbwrap: lmdbwrap.o 
	$(CXX) $(CXXFLAGS) $^ -llmdb -o $@


-include *.d