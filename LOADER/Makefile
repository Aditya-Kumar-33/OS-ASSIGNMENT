all:
	gcc  -m32 -no-pie -nostdlib -o fib fib.c
	gcc -m32 -o smartloader smartloader.c -lm

clean:
	-@rm -f fib smartloader
