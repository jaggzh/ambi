CFLAGS=-Wall
LIBS=-lpulse-simple -lpulse

all: run

run: ambi
	@echo "Test run (NOT continuous mode)"
	@echo "Running ambi to evaluate 3 seconds of audio..."
	./ambi -s 3 -p
	@# 1 sec, peak volume only

ambi: main.c
	cc $(CFLAGS) $^ $(LIBS) -o ambi

vi:
	vim \
		Makefile \
		main.c \
		README.md
