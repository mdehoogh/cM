#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

typedef struct{
	int a;
	int b;
}X;

typedef struct{
	int c;
	int d;
}Y;	

void* allocate(){


	void* ptr=malloc(sizeof(X)+sizeof(Y));

	((X*)ptr)->a=1;
	((X*)ptr)->b=2;

	void* y=((char*)ptr)+sizeof(Y);

	((Y*)y)->c=3;
	((Y*)y)->d=4;

	return y;
}

int main(){


	Y* y=(Y*)allocate();

	printf("Y c=%i,d=%i",y->c,y->d);

	return 0;

}