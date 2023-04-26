#include <stdio.h>
#include <stdlib.h>

int main() {
    char s1[] = "alpha";
    char s2[] = "beta";
    char filename[] = "output.txt";
    FILE* file;

    // open the output file for writing in binary mode
    file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error: could not open file for writing.\n");
        return 1;
    }

    // write the strings to the file separated by a comma
    fwrite(s1, sizeof(char), strlen(s1), file);
    fputc(',', file);
    fwrite(s2, sizeof(char), strlen(s2), file);

    // close the file
    fclose(file);

    return 0;
}