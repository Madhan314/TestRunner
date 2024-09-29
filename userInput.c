#include <stdio.h>

int main() {
    int n;
    //printf("Enter the number of terms in the Fibonacci sequence: ");
    scanf("%d", &n);

    int first = 0, second = 1, next;

    //printf("Fibonacci Sequence:\n");
    for (int i = 1; i <= n; i++) {
        if (i == 1) {
            printf("%d ", first);
            continue;
        }
        if (i == 2) {
            printf("%d ", second);
            continue;
        }
        next = first + second;
        first = second;
        second = next;
        printf("%d ", next);
    }
    
    //printf("\n");
    return 0;
}
