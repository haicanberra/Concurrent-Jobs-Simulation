# CCID: hmhoang
# Name : Hai Hoang
# Student ID: 1624290
# References: APUE 3/E Textbook, Linux/C manual page (man7.org) and resources on eClass.
CC = gcc
CFLAGS = -g -pthread

all: a4w22

a4w22: a4w22.c simulator.c
		$(CC) $(CFLAGS) -o a4w22 a4w22.c simulator.c
		
clean:
		rm -rf a4w22 *.o

tar:
		tar -cvf Hoang-a4.tar a4w22.c a4w22.h simulator.c simulator.h Makefile Report.pdf
