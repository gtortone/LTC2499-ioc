LIBS=-lm

all: clean ltc2499-test

ltc2499-test: ltc2499-test.c
	$(CC) -O2 -Wall ltc2499-test.c -o ltc2499-test $(LIBS)

clean:
	rm -f ltc2499-test

