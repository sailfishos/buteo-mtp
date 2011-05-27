all : mtpfs

all : libmtpfsdriver.a

mtpfs : main.o mtpfsdriver.o
	gcc main.o mtpfsdriver.o -o mtpfs

libmtpfsdriver.a : mtpfsdriver.o
	ar rcs libmtpfsdriver.a mtpfsdriver.o

main.o : main.cpp
	gcc -I /usr/src/kernel-headers/include -Wall -fPIC -c main.cpp

mtpfsdriver.o : mtpfsdriver.cpp
	gcc -I /usr/src/kernel-headers/include -Wall -fPIC -c mtpfsdriver.cpp

clean :
	rm -f *.o *.so mtpfs *~
