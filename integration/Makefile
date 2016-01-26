CPPFLAGS=-I../include -O0 -g3 -Wall -std=c++11 -fPIC -pthread
GCCFLAGS=-I../include -O0 -g3 -Wall -std=c++11 -fPIC -pthread
ASAN=-lasan
PLATFORM=linux_x64_debug
TESTS=BrutalConnectSuccess ConnectSuccess JoinSuccess LoginSuccess TrackOccupantsSuccess TrackMessageSuccess TrackAttributesSuccess TisztaTeszta
LINKFLAGS="-L../build/$(PLATFORM)/libs/uv/lib" -std=c++11 "-L../build/$(PLATFORM)/libs/tinyxml2/lib" "-L../build/$(PLATFORM)/make/output/lib" -lunionclientlibrary -ltinyxml2 -luv -lpthread -lrt $(ASAN)

all: clean $(TESTS)


BrutalConnectSuccess: BrutalConnectSuccess.o
	g++ -o $@ $< $(LINKFLAGS)
ConnectSuccess: ConnectSuccess.o
	g++ -o $@ $< $(LINKFLAGS)
JoinSuccess: JoinSuccess.o
	g++ -o $@ $<  $(LINKFLAGS)

LoginSuccess: LoginSuccess.o
	g++ -o $@ $< $(LINKFLAGS)

TrackOccupantsSuccess: TrackOccupantsSuccess.o
	g++ -o $@ $< $(LINKFLAGS)
	
TrackAttributesSuccess: TrackAttributesSuccess.o
	g++ -o $@ $< $(LINKFLAGS)

TrackMessageSuccess: TrackMessageSuccess.o
	g++ -o $@ $< $(LINKFLAGS)

TisztaTeszta: TisztaTeszta.o
	g++ -o $@ $< $(LINKFLAGS)

UnionTest: UnionTest.o
	g++ -o $@ $< $(LINKFLAGS)

clean:
	rm -rf $(TESTS) UnionTest *.o
