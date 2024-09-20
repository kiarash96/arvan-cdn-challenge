repairmen: repairmen.c repairmen.h
	cc repairmen.c -o repairmen -O2 -Wall

clean:
	rm -f repairmen

run: repairmen
	./repairmen
