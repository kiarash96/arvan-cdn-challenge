CFLAGS = -O2 -Wall

repairmen: repairmen.o main.c
	cc $(CFLAGS) -o repairmen repairmen.o main.c

repairmen.o: repairmen.c repairmen.h
	cc $(CFLAGS) -c repairmen.c

clean:
	rm -f repairmen.o repairmen

run: repairmen
	./repairmen $(TARGETS)

