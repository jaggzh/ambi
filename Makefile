CFLAGS=-Wall
LIBS=-lpulse-simple -lpulse

all: run

run: ambi
	./ambi -q -c -s .1 -p
	# quiet, continuous, 1 sec, peak volume

ambi: main.c
	cc $(CFLAGS) $^ $(LIBS) -o ambi

vi:
	vim \
		Makefile \
		main.c \
		README.md
