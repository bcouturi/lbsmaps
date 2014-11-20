CXXFLAGS = -O2 -g -Wall -pthread 
OBJS =	ProcessTable.o lbsmaps.o
LIBS = -lboost_regex 

TARGET = lbsmaps

$(TARGET):	$(OBJS)
	$(CXX) -static -pthread -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

