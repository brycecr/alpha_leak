#include <stdlib.h>


int main() {
    int *arr[10];
    int i;

    for(i = 0; i < 10; i++) {
        int *ptr = malloc(sizeof(int));
        arr[i] = ptr;
    }

    for(i = 2; i < 10; i += 2) {
	arr[i] = realloc(arr[i], sizeof(int) * 2);
    }

    printf("Done assigning... Freeing...\n");

    for (i = 1; i < 10; i++) {
        free(arr[i]);
    }

    return 0;
}
