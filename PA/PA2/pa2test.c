#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//Nicholas Wroblewski

#define MAX_BUFFER_SIZE 1024

int main(int argc, char **argv){
	if(argc != 2){
		printf("Invalid input. Type ./a.out filename to run.\n");
		return -1;
	}
	FILE *fp = fopen(argv[1], "r+"); //read the file
	if(!fp){
		printf("%s\n", "File open error.");
		return -1;
	}
	char rws_set; 
	int ch; //for clearing stdin after a scanf 
	while(1){
		printf("Option (r for read, w for write, s for seek): ");
		if(scanf(" %c", &rws_set) == EOF)          //write to rws_set and check for Ctrl-D
			goto EOF_EXIT;
		while ((ch = getchar()) != '\n' && ch != EOF); //https://stackoverflow.com/questions/40554617/while-getchar-n clear stdin
		if(rws_set == 'r'){
			int buffer_size;
			printf("Enter the number of bytes you want to read: ");
			if(scanf("%d", &buffer_size) == EOF) 	//again, check for Ctrl-D
				goto EOF_EXIT;
			while ((ch = getchar()) != '\n' && ch != EOF);
			if(buffer_size > MAX_BUFFER_SIZE)
				return -1;
			char *read_buffer = (char *) malloc(buffer_size * sizeof(char)); 	//allocate enough heap space to read n characters
			fread(read_buffer, sizeof(char), buffer_size, fp); 					//read it to the buffer
			puts(read_buffer); 													//print
		}
		else if(rws_set == 'w'){
			printf("Enter the string you want to write: ");
			char write_buffer[MAX_BUFFER_SIZE]; 				//could dynamically allocate it but I'm lazy
			if(!fgets(write_buffer, sizeof(write_buffer), stdin))
				goto EOF_EXIT;
			char *ptr = strchr(write_buffer, '\n'); 			//find where the user pressed enter, as this marks the end of the buffer
			*ptr = '\0'; 										//make it a proper string
			fprintf(fp, "%s", write_buffer); 					//write to the file
		}
		else if(rws_set == 's'){
			printf("Enter an offset value: ");
			int offset;
			if(scanf("%d", &offset) == EOF) 	//again, check for Ctrl-D
				goto EOF_EXIT;
			while ((ch = getchar()) != '\n' && ch != EOF); 	//clear the scanf buffer
			if(offset > MAX_BUFFER_SIZE)
				return -1;
			int whence = -1;
			while(whence < 0 || whence > 2){
				printf("Enter a value for whence (0 for SEEK_SET, 1 for SEEK_CUR, 2 for SEEK_END): ");
				if(scanf("%d", &whence) == EOF) 	//again, check for Ctrl-D
					goto EOF_EXIT;
				while ((ch = getchar()) != '\n' && ch != EOF); 	//clear the scanf buffer
			}
			fseek(fp, offset, whence);
		}
	}
	EOF_EXIT:
		fclose(fp);
		return 0;
}
