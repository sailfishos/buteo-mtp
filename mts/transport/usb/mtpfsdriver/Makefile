all : mtpfs

all : libmtpfsdriver.a

mtpfs : main.o mtpfsdriver.o
	gcc main.o mtpfsdriver.o -o mtpfs

libmtpfsdriver.a : mtpfsdriver.o
	ar rcs libmtpfsdriver.a mtpfsdriver.o

main.o : main.c
	gcc -I /usr/src/kernel-headers/include -Wall -fPIC -c main.c

mtpfsdriver.o : mtpfsdriver.c
	gcc -I /usr/src/kernel-headers/include -Wall -fPIC -c mtpfsdriver.c

clean :
	rm -f *.o *.so mtpfs *~
