CXX=g++
CXXFLAGS=-std=c++14 -O3 -Wall -lpthread


all: cantx canprint cangw cansim


cantx: cansocket.o cantx.o
	$(CXX) $(CXXFLAGS) cansocket.o cantx.o -o cantx
	@echo "Build finished"

canprint: cansocket.o canprint.o
	$(CXX) $(CXXFLAGS) cansocket.o canprint.o -o canprint
	@echo "Build finished"

cangw: cansocket.o udpsocket.o cangw.o
	$(CXX) $(CXXFLAGS) cansocket.o udpsocket.o cangw.o -o cangw
	@echo "Build finished"

cansim: timer.o udpsocket.o cansim.o
	$(CXX) $(CXXFLAGS) timer.o udpsocket.o cansim.o -o cansim
	@echo "Build finished"


timer.o: timer.cpp timer.h
	$(CXX) -c $(CXXFLAGS) timer.cpp

cansocket.o: cansocket.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cansocket.cpp

udpsocket.o: udpsocket.cpp udpsocket.h
	$(CXX) -c $(CXXFLAGS) udpsocket.cpp

cantx.o: cantx.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) cantx.cpp

canprint.o: canprint.cpp cansocket.h
	$(CXX) -c $(CXXFLAGS) canprint.cpp

cangw.o: cangw.cpp cansocket.h udpsocket.h priority.h
	$(CXX) -c $(CXXFLAGS) cangw.cpp

cansim.o: cansim.cpp udpsocket.h priority.h
	$(CXX) -c $(CXXFLAGS) cansim.cpp


clean:
	-rm *o cantx canprint cangw cansim
