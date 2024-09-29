#include <stdio.h>
#include <string.h>

int main() {
    FILE *file = fopen("userInput.txt", "w");
    if (file == NULL) {
        printf("Error opening file.\n");
        return 1;
    }

    char input[256];

    printf("Enter lines of text (type 'terminate' to stop):\n");

    while (1) {
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "terminate") == 0) {
            break;
        }

        //printf("%s\n", input);
        fprintf(file, "%s\n", input);
    }

    fclose(file);

    //printf("All lines written to 'userInput.txt'.\n");

    return 0;
}

