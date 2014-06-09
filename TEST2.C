#include <stdlib.h>


int main() {
    int *arr[10];
    int i;

    for(i = 0; i < 10; i++) {
        int *ptr = malloc(sizeof(int));
        arr[i] = ptr;
    }

    printf("Done assigning... Freeing...\n");

    for (i = 0; i < 10; i++) {
        free(arr[i]);
    }

    return 0;
}
