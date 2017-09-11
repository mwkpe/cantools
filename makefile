CXX=g++-4.9
CXXFLAGS=-std=c++14 -O3 -Wall -lpthread


all: cantx canprint cangw


cantx: cansocket.o cantx.o
	$(CXX) $(CXXFLAGS) cansocket.o cantx.o -o cantx
	@echo "Build finished"

canprint: cansocket.o canprint.o
	$(CXX) $(CXXFLAGS) cansocket.o canprint.o -o canprint
	@echo "Build finished"

cangw: cansocket.o cangw.o
	$(CXX) $(CXXFLAGS) cansocket.o cangw.o -o cangw
	@echo "Build finished"


cansocket.o: cansocket.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cansocket.cpp

cantx.o: cantx.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cantx.cpp

canprint.o: cantx.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) canprint.cpp

cangw.o: cantx.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cangw.cpp


clean:
	-rm *o cantx canprint cangw
