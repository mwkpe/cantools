CXX=g++
CXXFLAGS=-std=c++14 -O3 -Wall -lpthread


all: cantx canprint cangw cangw_simplex


cantx: cansocket.o cantx.o
	$(CXX) $(CXXFLAGS) cansocket.o cantx.o -o cantx
	@echo "Build finished"

canprint: cansocket.o canprint.o
	$(CXX) $(CXXFLAGS) cansocket.o canprint.o -o canprint
	@echo "Build finished"

cangw: cansocket.o udpsocket.o cangw.o
	$(CXX) $(CXXFLAGS) cansocket.o udpsocket.o cangw.o -o cangw
	@echo "Build finished"

cangw_simplex: cansocket.o udpsocket.o cangw_simplex.o
	$(CXX) $(CXXFLAGS) cansocket.o udpsocket.o cangw_simplex.o -o cangw_simplex
	@echo "Build finished"


cansocket.o: cansocket.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cansocket.cpp

udpsocket.o: udpsocket.cpp udpsocket.h
	$(CXX) -c $(CXXFLAGS) udpsocket.cpp

cantx.o: cantx.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cantx.cpp

canprint.o: cantx.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) canprint.cpp

cangw.o: cantx.cpp cansocket.h udpsocket.h
	$(CXX) -c $(CXXFLAGS) cangw.cpp

cangw_simplex.o: cantx.cpp cansocket.h udpsocket.h
	$(CXX) -c $(CXXFLAGS) cangw_simplex.cpp


clean:
	-rm *o cantx canprint cangw
