CFLAGS = -O2 -Wall

repairmen: barrier.o repairmen.o main.c repairmen.h barrier.h
	cc $(CFLAGS) -o repairmen barrier.o repairmen.o main.c

repairmen.o: repairmen.c repairmen.h barrier.h
	cc $(CFLAGS) -c repairmen.c

barrier.o: barrier.c barrier.h
	cc $(CFLAGS) -c barrier.c

clean:
	rm -f barrier.o repairmen.o repairmen test_repairmen test_barrier

run: repairmen
	./repairmen $(TARGETS)

test_repairmen: barrier.o repairmen.o test_repairmen.c barrier.h repairmen.h
	cc $(CFLAGS) -o test_repairmen barrier.o repairmen.o test_repairmen.c munit/munit.c

test_barrier: barrier.o test_barrier.c barrier.h
	cc $(CFLAGS) -o test_barrier -lpthread barrier.o test_barrier.c munit/munit.c

test: test_repairmen test_barrier
	./test_repairmen
	./test_barrier

