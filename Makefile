
FLAGS = -lpthread -std=c++11

SRCS = main.cc map_engine.cc protocol_parser.cc reactor.cc threadpool.cc write_ahead_log.cc connect_item.cc

OBJS = $(SRCS:.cc=.o)

TARGET = kvs

$(TARGET): $(OBJS)
	g++ -o $(TARGET) $(OBJS) $(FLAGS)

%.o: %.cc
	g++ -c $< -o $@ $(FLAGS)

.PHONY: clean

clean:
	rm -f $(OBJS) $(TARGET)

