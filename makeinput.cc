#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
  if (argc != 3) {
  	fprintf(stderr, "Syntax: %s numbers+filename\n", argv[0]);
  	exit(1);
  }
  
  FILE* output = fopen(argv[2],"w");

  srand(time(0));
  for (int i=0; i<atoi(argv[1]); i++) {
  	long long v = rand();
  	for (int j=0; j<3; j++)
  	  v = (v<<16)+rand();
  	fprintf(output,"%lld\n",v);
  }

  return 0;
}
