compile: user1.c user2.c 
	gcc user1.c -o user1 -lpthread
	gcc user2.c -o user2 -lpthread
run1: compile
	./user1
run2: compile
	./user2
clean:
	rm -f user1 user2