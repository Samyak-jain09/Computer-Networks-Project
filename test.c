#include <stdio.h>
#include <string.h>

int main() {
    FILE *fp;
    char filename[] = "name.txt";
    char line[100];
    char *token;
    int offset = 0;
    
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file.\n");
        return 1;
    }
    int final_offset;
    while (fgets(line, sizeof(line), fp)) {
        // remove trailing newline character
        line[strcspn(line, "\n")] = 0;
        
        // check if line ends with full stop
        if (line[strlen(line)-1] == '.') {
            line[strlen(line)-1] = '\0';
        }
        
        // tokenize the line based on comma delimiter
        token = strtok(line, ",");
        while (token != NULL) {
            int len = strlen(token);
            
            printf("Token: %s Offset: %d\n", token, offset);
            final_offset=offset;
            offset += len;
            
            // skip the comma, if present
            if (offset < strlen(line) && line[offset] == ',') {
                offset++;
            }
            
            token = strtok(NULL, ",");
        }
    }
    
    printf("Last offset = %d\n",final_offset );
    fclose(fp);
    return 0;
}
