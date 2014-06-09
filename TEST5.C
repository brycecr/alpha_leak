#include <stdlib.h>

int main() {
   void* mall[10];

   for (int i=0; i < 10; ++i) {
      mall[i] = malloc(10*i);
   }
   for (int i=0; i < 5; i++) {
      free(mall[i]);
   }
   for (int i=0; i < 3; ++i) {
      free(mall[i]);
   }
}
