librsocket.a: rsocket.o
	ar -rcs librsocket.a rsocket.o

rsocket.o: rsocket.c rsocket.h
	gcc -Wall -c rsocket.c

clean:
	rm user1_obj.o user2_obj.o user1 user2 rsocket.o librsocket.a

# user: user1_obj.o  user2_obj.o librsocket.a
# 	gcc -Wall user1_obj.o -L. -lrsocket -lpthread -o user1
# 	gcc -Wall user2_obj.o -L. -lrsocket -lpthread -o user2

# user1_obj.o: user1.c user2.c rsocket.h
# 	gcc -Wall -c user1.c -o user1_obj.o
# 	gcc -Wall -c user2.c -o user2_obj.o
