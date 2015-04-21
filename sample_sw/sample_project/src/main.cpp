#include "stuff1/file2.h"
#include "stuff1/file2.h"
#include "stuff2/file3.h"
#include "stuff2/stuff3/file4.h"
#include "stuff2/stuff3/file5.h"
#include <cstdio>


int main() {
	printf("Hello world, I'm the program you just built.\n");
#ifdef COOL_VERSION
	printf("*** Supercool version ***\n");
#endif
}
