
#include "multi-lookup.h"
#include "util.h"
#include "sys/time.h"

void push(stack_struct *stack, char *name){ //push a hostname to the stack
	//printf("Push %s\n", name);
	sem_wait(stack->empty_count); //check if there are spaces available for writing
	sem_wait(stack->mutex); //don't allow writing or reading on the buffer
	if (stack->top < 0){
		return;
	}
	strcpy(stack->array[stack->top++], name); //put element on stack
	sem_post(stack->mutex); //unlock the buffer
	sem_post(stack->fill_count); //add to number of filled spots
}

char *pop(stack_struct *stack){ //get hostname from stack
	//print(stack);
	char *name;
	sem_wait(stack->fill_count); //check for filled spots
	sem_wait(stack->mutex); //lock the buffer
	if (stack->top < 1){ //check if there are still elements to read
		sem_post(stack->mutex); //if not, make sure to post mutex
		return "\0"; //empty string signals the thread to exit
	}
	name = stack->array[--(stack)->top]; //read hostname
	sem_post(stack->mutex); //unlock buffer
	sem_post(stack->empty_count); //increment number of empty spots
	return name;
}

void* producer(void *ptr){     	 // place element on the top of the array
	char *input_filename;
	char hostname[MAX_NAME_LENGTH];
	FILE *fp;
	while((((pthread_create_request_args*)ptr)->num_files - (*((pthread_create_request_args*)ptr)->fp)) > 0){ //while there are still files to be read, doesn't need to be locked 
		sem_wait(((pthread_create_request_args*)ptr)->args_mutex);
		input_filename = ((pthread_create_request_args*)ptr)->argv[*((pthread_create_request_args*)ptr)->fp + 5];
		(*((pthread_create_request_args*)ptr)->fp)++;
		sem_post(((pthread_create_request_args*)ptr)->args_mutex);
		//printf("-%s-\n", input_filename);
		fp = fopen(input_filename, "r");
		if(fp){
			while(fscanf(fp, "%s\n", hostname) != EOF){ //read until end of file
				sem_wait(((pthread_create_request_args*)ptr)->outfp_mutex);
				fprintf(((pthread_create_request_args*)ptr)->req_outfile, "Requesting: %s\n", hostname); //the request log doesn't really matter, we don't need to lock it
				sem_post(((pthread_create_request_args*)ptr)->outfp_mutex);
				push(((pthread_create_request_args*)ptr)->stack, hostname); //push read hostname to stack
			}
			sem_wait(((pthread_create_request_args*)ptr)->stdout_mutex);
			printf("File completely serviced: %s\n", input_filename);
			sem_post(((pthread_create_request_args*)ptr)->stdout_mutex);
			fclose(fp);
		}
		else{
			perror("fopen");
			//printf("invalid file: %s\n", input_filename);
		}

	}
	return NULL;
}

void* consumer(void *ptr){ 
	char hostname[MAX_NAME_LENGTH]; //stores hostname read from stack
	char ip_addr[MAX_IP_LENGTH]; 
	int num_resolved = 0; //number of hostnames that are resolved
	while (1){ //consumer threads execute unitl signaled to exit; exits 3 lines below on the break
		strcpy(hostname, pop(((pthread_create_resolve_args*)ptr)->stack)); //copy value from stack to hostname
		if(!strcmp("\0", hostname)){ //check if thread should exit
			break;
		}
		if(dnslookup(hostname, ip_addr, 32) == -1){ //get ip addr
			strcpy(ip_addr, NOT_RESOLVED);
			printf("-%s-\n", hostname);
		}
		else{
			num_resolved++; //increment number of resolved on success
		}
		
		sem_wait(((pthread_create_resolve_args*)ptr)->outfp_mutex); //lock, write, and unlock output file
		fprintf(((pthread_create_resolve_args*)ptr)->res_outfile, "%s, %s\n", hostname, ip_addr);
		sem_post(((pthread_create_resolve_args*)ptr)->outfp_mutex);
	}
	sem_wait(((pthread_create_resolve_args*)ptr)->stdout_mutex);
	printf("thread 0x%lx resolved %d hostnames.\n", pthread_self(), num_resolved);
	sem_post(((pthread_create_resolve_args*)ptr)->stdout_mutex);
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
	struct timeval start, end; //for logging
	gettimeofday(&start, NULL);
	sem_t 	args_mutex, //various semaphores
			resolve_log_mutex, 
			stack_mutex, 
			fill_count, 
			empty_count,
			stdout_mutex,
			request_log_mutex;

	sem_init(&fill_count, 0, 0); //stack starts empty;
	sem_init(&empty_count, 0, STACK_SIZE); //number of filled slots is zero
	sem_init(&stack_mutex, 0, 1); //set all mutexes to unlocked
	sem_init(&args_mutex, 0, 1);
	sem_init(&resolve_log_mutex, 0, 1);
	sem_init(&stdout_mutex, 0, 1);
	sem_init(&request_log_mutex, 0, 1);

	int num_requestor = atoi(argv[1]);
	int num_resolver = atoi(argv[2]);

	int fp = 0, num_files = argc - 5; //fp keeps track of the current input file, this needs to be locked. 
	if(num_files > 100){
		printf("Maximum of 100 files. %d were inputted. \n", num_files);
		return -1;
	}
	stack_struct stack; //stack that will shared between all threads
	stack.top = 0;
	stack.empty_count = &empty_count; //point to semaphores
	stack.fill_count = &fill_count;
	stack.mutex = &stack_mutex;

	pthread_t producer_threads[num_requestor]; //make consumer and producer threads
	pthread_t consumer_threads[num_resolver];

	pthread_create_resolve_args resolve_args[num_resolver]; //and their argument structs. 
	pthread_create_request_args request_args[num_resolver];

	FILE *resolve_outfile; //open resolve file
	resolve_outfile = fopen(argv[4], "w+");
	if(!resolve_outfile){
		perror("fopen");
		printf("Could not open %s.\n", argv[4]);
		return -1;
	}
	FILE *request_outfile; //open request file
	request_outfile = fopen(argv[3], "w+");
	if(!request_outfile){
		perror("fopen");
		printf("Could not open %s.\n", argv[3]);
		return -1;
	}
	//set pointers and start the threads. stack is shared between threads
	for(int i = 0; i < num_requestor; i++){ 
		request_args[i].num_files		= num_files;
		request_args[i].fp				= &fp;
		request_args[i].argv 			= argv;
		request_args[i].args_mutex		= &args_mutex;
		request_args[i].stack 			= &stack;
		request_args[i].stdout_mutex	= &stdout_mutex;
		request_args[i].outfp_mutex		= &request_log_mutex;
		request_args[i].req_outfile		= request_outfile;
	}
	for(int i = 0; i < num_resolver; i++){
		resolve_args[i].res_outfile 	= resolve_outfile;
		resolve_args[i].stack		 	= &stack;
		resolve_args[i].outfp_mutex 	= &resolve_log_mutex;
		resolve_args[i].stdout_mutex	= &stdout_mutex;
	}	

	for(int i = 0; i < num_requestor; i++){
		pthread_create(&producer_threads[i], NULL, producer, &request_args[i]); 
	}
	for(int i = 0; i < num_resolver; i++){
		pthread_create(&consumer_threads[i], NULL, consumer, &resolve_args[i]); 
	}

	for(int i = 0; i < num_requestor; i++){
		pthread_join(producer_threads[i], NULL);
	}
	// once all producer threads exit, all the consumers can be unlocked. If they try to read from the stack
	// ant its empty they will exit
	for(int i = 0; i < num_resolver; i++){
		sem_post(stack.fill_count);
	}
	//wait for consumers to exit
	for(int i = 0; i < num_resolver; i++){
 		pthread_join(consumer_threads[i], NULL);
	}

	sem_destroy(&stack_mutex); //cleanup semaphores
	sem_destroy(&empty_count);
	sem_destroy(&fill_count);
	sem_destroy(&args_mutex);
	sem_destroy(&stdout_mutex);
	sem_destroy(&request_log_mutex);
	sem_destroy(&resolve_log_mutex);

	fclose(request_outfile); //close output files
	fclose(resolve_outfile);

	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; //https://stackoverflow.com/questions/2533774/measuring-execution-time-of-a-call-to-system-in-c
	printf("\n./multi-lookup: total time is %f seconds\n", elapsed);
	return 0;
}

