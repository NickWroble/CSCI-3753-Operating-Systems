
#include "multi-lookup.h"
#include "util.h"
#include "sys/time.h"

void* producer(void *ptr){     	 // place element on the top of the array
	char *input_filename;
	char hostname[MAX_NAME_LENGTH];
	FILE *fp;
	while((*((pthread_create_request_args*)ptr)->num_files - (*((pthread_create_request_args*)ptr)->fp)) > 0){
			sem_wait(((pthread_create_request_args*)ptr)->args_mutex);
			input_filename = ((pthread_create_request_args*)ptr)->argv[*((pthread_create_request_args*)ptr)->fp + 5];
			(*((pthread_create_request_args*)ptr)->fp)++;
			sem_post(((pthread_create_request_args*)ptr)->args_mutex);
			fp = fopen(input_filename, "r");
			if(fp){
				while(fscanf(fp, "%s\n", hostname) != EOF){
					fprintf(((pthread_create_request_args*)ptr)->req_outfile, "Requesting: %s\n", hostname);
					sem_wait(((pthread_create_request_args*)ptr)->stack->empty_count); //check if there are spaces available for writing
					sem_wait(((pthread_create_request_args*)ptr)->stack->mutex); //don't allow writing or reading on the buffer
					if (((pthread_create_request_args*)ptr)->stack->top < 0){
						printf("Stack pointer error-> Value is %d-> Must be between 0 and %d.\n", ((pthread_create_request_args*)ptr)->stack->top, STACK_SIZE);
						return NULL;
					}
					strcpy(((pthread_create_request_args*)ptr)->stack->array[((pthread_create_request_args*)ptr)->stack->top++], hostname);
					//print(((pthread_create_request_args*)ptr)->stack);
					sem_post(((pthread_create_request_args*)ptr)->stack->mutex); //unlock the buffer
					sem_post(((pthread_create_request_args*)ptr)->stack->fill_count); //add to number of filled spots
				}
				printf("File completely serviced: %s\n", input_filename);
				fclose(fp);
			}
			else{
				perror("fopen");
				printf("invalid file: %s\n", input_filename);
			}

		}
	return NULL;
}

void* consumer(void *ptr){ 
	((pthread_create_resolve_args*)ptr)->tid = (long int) pthread_self();
	char *hostname = "";// = "";
	int num_resolved = 0;
		while (1){
			sem_wait(((pthread_create_resolve_args*)ptr)->stack->fill_count); //check for filled spots
			sem_wait(((pthread_create_resolve_args*)ptr)->stack->mutex); //lock the buffer
			if (((pthread_create_resolve_args*)ptr)->stack->top < 1){
				sem_post(((pthread_create_resolve_args*)ptr)->stack->fill_count);
				sem_post(((pthread_create_resolve_args*)ptr)->stack->mutex);
				break;
			}
			hostname = ((pthread_create_resolve_args*)ptr)->stack->array[--(((pthread_create_resolve_args*)ptr)->stack)->top];
			//print(((pthread_create_resolve_args*)ptr)->stack);
			sem_post(((pthread_create_resolve_args*)ptr)->stack->mutex); //unlock buffer
			sem_post(((pthread_create_resolve_args*)ptr)->stack->empty_count); //increment number of empty spots
			sem_wait(((pthread_create_resolve_args*)ptr)->outfp_mutex);
			char ip_addr[MAX_IP_LENGTH];
			if(dnslookup(hostname, ip_addr, 32) == -1){
				strcpy(ip_addr, NOT_RESOLVED);
				printf("%s\n", hostname);
			}
			else{
				num_resolved++;
			}
			fprintf(((pthread_create_resolve_args*)ptr)->res_outfile, "%s, %s\n", hostname, ip_addr);
			sem_post(((pthread_create_resolve_args*)ptr)->outfp_mutex);
		}
	printf("thread 0x%lx resolved %d hostnames.\n", pthread_self(), num_resolved);
	return NULL;
}

void print(stack_struct * s) {        //print stack
	printf("buffer:    ");
	for(int i = 0; i < STACK_SIZE; i++)
		printf("%s ", s->array[i]);
	printf("\n");
	printf("top: %d\n", s->top);
}

int main(int argc, char *argv[]){
//int main(){
	if(argc < 6){
		printf("Not enough input arguments. Run with ./a.out num_req_thread num_res_threads request.log resolve.log [input.file]\n");
		return -1;
	}
	struct timeval start, end;
	gettimeofday(&start, NULL);
	sem_t stdout_mutex, args_mutex, resolve_log_mutex;//, resolve_file, request_file;//, resolve_mutex, request_mutex;
	sem_t mutex, fill_count, empty_count;
	sem_init(&fill_count, 0, 0);
	sem_init(&empty_count, 0, STACK_SIZE);
	sem_init(&mutex, 0, 1);
	sem_init(&stdout_mutex, 0, 1);
	sem_init(&args_mutex, 0, 1);
	sem_init(&resolve_log_mutex, 0, 1);

	int num_requestor = atoi(argv[1]);
	int num_resolver = atoi(argv[2]);
	char resolve_logname[MAX_NAME_LENGTH];
	char request_logname[MAX_NAME_LENGTH];
	strcpy(request_logname, argv[3]);
	strcpy(resolve_logname, argv[4]);

	if(num_requestor > 10){
		printf("The number of requestor threads must be set to 10.\n");
		return -1;
	}
	if(num_resolver > 10){
		printf("The number of resolver threads must be set to 10.\n");
		return -1;
	}

	int fp = 0, num_files = argc - 5;
	if(num_files > 100){
		printf("Maximum of 100 files. %d were inputted. \n", num_files);
		return -1;
	}



	stack_struct stack; 
	for(int i = 0; i < STACK_SIZE; i++)
	 	strcpy(stack.array[i], "");
	stack.top = 0;
	stack.empty_count = &empty_count;
	stack.fill_count = &fill_count;
	stack.mutex = &mutex;

	pthread_t producer_threads[num_requestor];
	pthread_t consumer_threads[num_resolver];

	pthread_create_resolve_args resolve_args[num_resolver];
	pthread_create_request_args request_args[num_resolver];

	FILE *resolve_outfile;
	resolve_outfile = fopen(resolve_logname, "w+");
	if(!resolve_outfile){
		perror("fopen");
		printf("Could not open %s.\n", resolve_logname);
		return -1;
	}
	FILE *request_outfile;
	request_outfile = fopen(request_logname, "w+");
	if(!request_outfile){
		perror("fopen");
		printf("Could not open %s.\n", request_logname);
		return -1;
	}
	for(int i = 0; i < num_requestor; i++){

		request_args[i].req_outfile		= request_outfile;
		request_args[i].stdout_mutex 	= &stdout_mutex;
		request_args[i].num_files		= &num_files;
		request_args[i].fp				= &fp;
		request_args[i].argv 			= argv;
		request_args[i].args_mutex		= &args_mutex;
		request_args[i].stack 			= &stack;
 		pthread_create(&producer_threads[i], NULL, producer, &request_args); 
	}
	for(int i = 0; i < num_resolver; i++){
		resolve_args[i].res_outfile 	= resolve_outfile;
		resolve_args[i].stack		 	= &stack;
		resolve_args[i].outfp_mutex 	= &resolve_log_mutex;
		resolve_args[i].num_files 		= &num_files;
 		pthread_create(&consumer_threads[i], NULL, consumer, &resolve_args); 
	}	
	for(int i = 0; i < num_requestor; i++){
		pthread_join(producer_threads[i], NULL);
	}
	printf("\n\n---------------------------------\n\n");
	for(int i = 0; i < STACK_SIZE; i++){
		sem_post(stack.fill_count);
	}
	for(int i = 0; i < num_resolver; i++){
 		pthread_join(consumer_threads[i], NULL);
	}

	sem_destroy(&mutex);
	sem_destroy(&empty_count);
	sem_destroy(&fill_count);
	sem_destroy(&stdout_mutex);
	sem_destroy(&args_mutex);
	fclose(request_outfile);
	fclose(resolve_outfile);

	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; //https://stackoverflow.com/questions/2533774/measuring-execution-time-of-a-call-to-system-in-c
	printf("\n./multi-lookup: total time is %f seconds\n", elapsed);
	return 0;
}

