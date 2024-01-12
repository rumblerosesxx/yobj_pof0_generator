clean:
	rm encoder

encoder: pof0gen.c
	gcc -o $@ $?
