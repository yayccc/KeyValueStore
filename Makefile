
kvs: main.o map_engine.o protocol_parser.o reactor.o threadpool.o write_ahead_log.o connect_item.o
	g++ -o kvs main.o map_engine.o protocol_parser.o reactor.o threadpool.o write_ahead_log.o connect_item.o -lpthread -std=c++11

main.o: main.cc
	g++ -c main.cc -o main.o -std=c++11

reactor.o: reactor.cc
	g++ -c reactor.cc -o reactor.o -std=c++11

write_ahead_log.o: write_ahead_log.cc
	g++ -c write_ahead_log.cc -o write_ahead_log.o -std=c++11

threadpool.o: threadpool.cc
	g++ -c threadpool.cc -o threadpool.o -std=c++11

connect_item.o: connect_item.cc
	g++ -c connect_item.cc -o connect_item.o -std=c++11

map_engine.o: map_engine.cc
	g++ -c map_engine.cc -o map_engine.o -std=c++11

protocol_parser.o: protocol_parser.cc
	g++ -c protocol_parser.cc -o protocol_parser.o -std=c++11

.PHONY:
clear:
	rm *.o kvs
