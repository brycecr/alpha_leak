#include <stdio.h>
#include <stdlib.h>

int main() {

	int *ptr;
	int *arr[10];
	for (int i=0; i<10; ++i) {
		ptr=malloc(200);
		arr[i] = ptr;
		*ptr=i;
		if (i % 2 == 0 && i != 0) {
		    arr[i-1] = (int *) malloc(sizeof(int));
		    *arr[i-1] = i*i;
		}
		    
	}
	free(arr[4]);
}
