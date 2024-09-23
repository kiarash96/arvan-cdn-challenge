CFLAGS = -O2 -Wall

repairmen: barrier.o repairmen.o main.c repairmen.h barrier.h
	cc $(CFLAGS) -o repairmen barrier.o repairmen.o main.c

repairmen.o: repairmen.c repairmen.h barrier.h
	cc $(CFLAGS) -c repairmen.c

barrier.o: barrier.c barrier.h
	cc $(CFLAGS) -c barrier.c

clean:
	rm -f barrier.o repairmen.o repairmen

run: repairmen
	./repairmen $(TARGETS)

