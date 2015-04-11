#include <stdio.h>

int calculate(int y) {
  int i, j; int total = 0;
  for (i = 1; i <= y; i++)
    for (j = 1; j <= i; j++)
      total += i/j;
  return total;
}

int main() {
  printf("result is %d\n", calculate(2350/2));
}
