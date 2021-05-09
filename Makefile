all: scheduler.o client.o hospitalA.o
		g++ -o scheduler scheduler.cpp
		g++ -o client client.cpp
		g++ -o hospitalA hospitalA.cpp
		g++ -o hospitalB hospitalB.cpp
		g++ -o hospitalC hospitalC.cpp

scheduler:
		./scheduler

client:
		./client

hospitalA:
		./hospitalA

hospitalB:
		./hospitalB

hospitalC:
		./hospitalC

clean:
		rm -f *.o scheduler client hospitalA hospitalB hospitalC
	