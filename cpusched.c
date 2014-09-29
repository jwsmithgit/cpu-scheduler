/*
 * cpusched.c
 *
 * Solution to A#3, CSC 360
 * Spring 2014
 *
 * Submission for Jacob Smith V00700979
 *
 * Skeleton code prepared by: Michael Zastre (University of Victoria)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_LINE_LENGTH 100

#define FCFS 0
#define PS   1
#define MLFQ 2
#define STRIDE 3

#define PRIORITY_LEVELS 4


/*
 * Stores raw event data from the input,
 * and has spots for per-task statistics.
 * You may want to modify this if you wish
 * to store other per-task statistics in
 * the same spot.
 */

typedef struct Task_t {
    int   arrival_time;
    float length;
    int   priority;

    float finish_time;
    int   schedulings;
    float cpu_cycles;

	// stride scheduling variables
	int stride;
	int meter;
} task_t; 


/*
 * Some function prototypes.
 */

void read_task_data(void);
void init_simulation_data(int);
void first_come_first_serve(void);
void stride_scheduling(int);
void priority_scheduling(void);
void mlfq_scheduling(int);
void run_simulation(int, int);
void compute_and_print_stats(void);


/*
 * Some global vars.
 */
int     num_tasks = 0;
task_t *tasks = NULL;


void read_task_data()
{
    int max_tasks = 2;
    int  in_task_num, in_task_arrival, in_task_priority;
    float in_task_length;
    

    assert( tasks == NULL );

    tasks = (task_t *)malloc(sizeof(task_t) * max_tasks);
    if (tasks == NULL) {
        fprintf(stderr, "error: malloc failure in read_task_data()\n");
        exit(1);
    }
   
    num_tasks = 0;

    /* Given the format of the input is strictly formatted,
     * we can used fscanf .
     */
    while (!feof(stdin)) {
        fscanf(stdin, "%d %d %f %d\n", &in_task_num,
            &in_task_arrival, &in_task_length, &in_task_priority);
        assert(num_tasks == in_task_num);
        tasks[num_tasks].arrival_time = in_task_arrival;
        tasks[num_tasks].length       = in_task_length;
        tasks[num_tasks].priority     = in_task_priority;

        num_tasks++;
        if (num_tasks >= max_tasks) {
            max_tasks *= 2;
            tasks = (task_t *)realloc(tasks, sizeof(task_t) * max_tasks);
            if (tasks == NULL) {
                fprintf(stderr, "error: malloc failure in read_task_data()\n");
                exit(1);
            } 
        }
    }
}


void init_simulation_data(int algorithm)
{
    int i;

    for (i = 0; i < num_tasks; i++) {
        tasks[i].finish_time = 0.0;
        tasks[i].schedulings = 0;
        tasks[i].cpu_cycles = 0.0;

		// initializing stride variables
		tasks[i].stride = 0;
		tasks[i].meter = 0;
    }
}


void first_come_first_serve() 
{
    int current_task = 0;
    int current_tick = 0;

    for (;;) {
        current_tick++;

        if (current_task >= num_tasks) {
            break;
        }

        /*
         * Is there even a job here???
         */
        if (tasks[current_task].arrival_time > current_tick-1) {
            continue;
        }

        tasks[current_task].cpu_cycles += 1.0;
        
        if (tasks[current_task].cpu_cycles >= tasks[current_task].length) {
            float quantum_fragment = tasks[current_task].cpu_cycles -
                tasks[current_task].length;
            tasks[current_task].cpu_cycles = tasks[current_task].length;
            tasks[current_task].finish_time = current_tick - quantum_fragment;
            tasks[current_task].schedulings = 1;
            current_task++;
            if (current_task > num_tasks) {
                break;
            }
            tasks[current_task].cpu_cycles += quantum_fragment;
        }
    }
}


// stride scheduling
void stride_scheduling(int quantum)
{
	int current_task = 0; // which data task we are currently handling
	tasks[current_task].schedulings ++; // will schedule first task always
    int current_tick = 0;

	int next_task = current_task + 1; // the next task that can enter the system
	int tasks_completed = 0; // how many tasks have run til completion, used to exit
	
	int tickets = 10000; // arbitrary number of tickets
	
	// set stride numbers for each task. 
	// stride is proportional to priority number
	int i;
	for (i = 0; i< num_tasks; i++) {
		tasks[i].stride = tickets/((tasks[i].priority+1)*50); 
	}
	
	/*int i;
	for (i = 0; i< num_tasks; i++) {
		printf("Arrival for task %d --- %d\n", i, tasks[i].arrival_time);
		printf("Priority for task %d --- %d\n", i, tasks[i].priority);
		printf("Length for task %d --- %f\n", i, tasks[i].length);
	}*/

    for (;;) {
        current_tick += quantum;


        // checks if first task has yet to arrive
        if (tasks[current_task].arrival_time > current_tick-quantum) {
            continue;
        } 

		// check if next task is ready to run
		if (next_task < num_tasks) {
			while (tasks[next_task].arrival_time <= current_tick-quantum) {
			
				next_task+=1;
				if (next_task >= num_tasks) {
					break;
				}
			}
		}

        tasks[current_task].cpu_cycles += quantum; // increment how long the task has ran for
		tasks[current_task].meter += tasks[current_task].stride; // increase the stride meter

		// task is finished running
        if (tasks[current_task].cpu_cycles >= tasks[current_task].length) {
            float quantum_fragment = tasks[current_task].cpu_cycles -
                tasks[current_task].length;
            tasks[current_task].cpu_cycles = tasks[current_task].length;
            tasks[current_task].finish_time = current_tick - quantum_fragment;
			tasks_completed ++;
			
			// we are done all tasks, exit
            if (tasks_completed >= num_tasks) {
                break;
            } 
			
			// rollback, the simulation pretends there is no downtime between different tasks
			current_tick -= quantum_fragment;
        }
		// if all tasks are not finished, need to pick a task for next loop
		int j;
		for (j=0; j<next_task; j++) {
			// if current task is finished, looking for any task to fill current task
			if (tasks[current_task].finish_time > 0.0 && tasks[j].finish_time <= 0.0) {
				current_task = j;
			}		
			// current task is filled, looking for lower meter
			else {
				if (tasks[current_task].meter > tasks[j].meter && tasks[j].finish_time <= 0.0) {
					current_task = j;
				} 
			}
		}
		tasks[current_task].schedulings ++;
		
    }

}

// some good ol' priority scheduling
void priority_scheduling()
{
	int current_task = 0; // which data task we are currently handling
	tasks[current_task].schedulings ++; // will schedule first task always
    int current_tick = 0; // tick simulation

	int next_task = current_task + 1; // next task that can possibly get scheduled
	int tasks_completed = 0; // how many tasks have run til completion

	/*int i;
	for (i = 0; i< num_tasks; i++) {
		printf("Arrival for task %d --- %d\n", i, tasks[i].arrival_time);
		printf("Priority for task %d --- %d\n", i, tasks[i].priority);
		printf("Length for task %d --- %f\n", i, tasks[i].length);
	}*/

    for (;;) {
        current_tick++;

		// check if we have moved through all tasks
        /*if (current_task >= num_tasks) {
            break;
        }*/

        // check for the first job
        if (tasks[current_task].arrival_time > current_tick-1) {
            continue;
        } 

		// check if next task is ready to run, while incase 2 objects appear at the same time
		if( next_task < num_tasks ){
			while (tasks[next_task].arrival_time <= current_tick-1) {
				

				// check if its priority is lower then the current priority
				if( tasks[next_task].priority < tasks[current_task].priority ){
					current_task = next_task;
					tasks[current_task].schedulings ++; // task has changed, scheduled
				}

				next_task+=1;
				if (next_task >= num_tasks) { // all tasks have entered the system
					break;
				}
			}
		}

		// increment how long the task has ran for
        tasks[current_task].cpu_cycles += 1.0;

		// task is finished running
        if (tasks[current_task].cpu_cycles >= tasks[current_task].length) {
            float quantum_fragment = tasks[current_task].cpu_cycles -
                tasks[current_task].length;
            tasks[current_task].cpu_cycles = tasks[current_task].length;
            tasks[current_task].finish_time = current_tick - quantum_fragment;
			tasks_completed ++;
			
			// we are done all tasks
            if (tasks_completed >= num_tasks) {
                break;
            } 
			// there are still tasks to complete
			// find next task with lowest priority
			else {
				int j = 0;
				for (j=0; j<next_task; j++) {
					// looking for any task to fill current task
					if (tasks[current_task].finish_time > 0.0 && tasks[j].finish_time <= 0.0) {
						current_task = j;
					}		
					// current task is filled, looking for lower priority
					else {
						if (tasks[current_task].priority > tasks[j].priority && tasks[j].finish_time <= 0.0) {
							current_task = j;
						} 
					}
				}
				tasks[current_task].schedulings ++; // task has changed, scheduled
			}
            tasks[current_task].cpu_cycles += quantum_fragment; // add quantum fragment to next task
        }
    }
}

// old priority scheduling
// swaps lower priority tasks to front of array
// rearranges original copy, will not work for assignment
/*void priority_scheduling2()
{
	int current_task = 0;
    int current_tick = 0;

	int next_task = current_task + 1;


    for (;;) {
        current_tick++;

		// check if we have moved through all tasks
        if (current_task >= num_tasks) {
            break;
        }


        //Is there even a job here???
        if (tasks[current_task].arrival_time > current_tick-1) {
            continue;
        }

		// check if next task is ready to run
		while (tasks[next_task].arrival_time <= current_tick-1) {
			if (next_task >= num_tasks) {
				break;
			}

			int temp_task = next_task;
			// traverse backwards through list, find right position for new task
			while (tasks[temp_task].priority < tasks[temp_task-1].priority){
				task_t *temp = (task_t *)malloc(sizeof(task_t));
				if (temp == NULL) {
					fprintf(stderr, "error: malloc failure in priority_scheduling()\n");
					exit(1);
    			}
				*temp = tasks[temp_task-1];
				tasks[temp_task-1] = tasks[temp_task];
				tasks[temp_task] = *temp;
				
				temp_task-=1;
				if (temp_task <= current_task) {
					break;
				}
			}

			next_task+=1;
		}

		// increment how long the task has ran for
        tasks[current_task].cpu_cycles += 1.0;
        

		// task is finished running
        if (tasks[current_task].cpu_cycles >= tasks[current_task].length) {
            float quantum_fragment = tasks[current_task].cpu_cycles -
                tasks[current_task].length;
            tasks[current_task].cpu_cycles = tasks[current_task].length;
            tasks[current_task].finish_time = current_tick - quantum_fragment;
            tasks[current_task].schedulings = 1;
            current_task++;
            if (current_task > num_tasks) {
                break;
            }
            tasks[current_task].cpu_cycles += quantum_fragment;
        }
    }
}*/

// enqueue a task into a looped list.
void enqueue_task(task_t* queue[], task_t* task, int* start_pos, int* queue_current_size, int queue_max_size){
	queue[(*start_pos+*queue_current_size)%queue_max_size] = task;
	*queue_current_size += 1;
}

// dequeue a task into a looped list.
task_t* dequeue_task(task_t* queue[], int* start_pos, int* queue_current_size, int queue_max_size){
	task_t* result = queue[(*start_pos)]; // need to modify start_pos, so grab the result now
	
	*start_pos = (*start_pos+1)%queue_max_size;
	*queue_current_size -= 1;
	
	return result;
}

void mlfq_scheduling(int quantum)
{
	
	int number_of_levels = 4; // arbitrary number of levels
	int size_of_queue = 100; // max size for task queues,
	int i, j;

	// lists for each queue
	// array for multi levels
	// array for queues
	// pointers for tasks
	task_t ***Q = (task_t ***)malloc(sizeof(task_t**) * number_of_levels);
	if (Q == NULL) {
        fprintf(stderr, "error: malloc failure in mlfq_scheduling()\n");
        exit(1);
    }
	for( i=0; i<number_of_levels; i++ ){
		Q[i] = (task_t **)malloc(sizeof(task_t*) * size_of_queue);
		if (Q[i] == NULL) {
		    fprintf(stderr, "error: malloc failure in mlfq_scheduling()\n");
		    exit(1);
		}
		for( j=0; j<size_of_queue; j++ ){
			Q[i][j] = (task_t *)malloc(sizeof(task_t));
			if (Q[i][j] == NULL) {
				fprintf(stderr, "error: malloc failure in mlfq_scheduling()\n");
				exit(1);
			}
		}
	}

	// pointers for the start of each queue
	// will point to the start of the list
	int *P = (int *)malloc(sizeof(int) * number_of_levels);
	if (P == NULL) {
        fprintf(stderr, "error: malloc failure in mlfq_scheduling()\n");
        exit(1);
    }
	for( i=0; i<number_of_levels; i++ ){
		P[i] = 0;
	}

	// size of each queue
	int *N = (int *)malloc(sizeof(int) * number_of_levels);
	if (N == NULL) {
        fprintf(stderr, "error: malloc failure in mlfq_scheduling()\n");
        exit(1);
    }
	for( i=0; i<number_of_levels; i++ ){
		N[i] = 0;
	}

	// set arbitrary time slice for each task. 
	int *TS = (int *)malloc(sizeof(int) * number_of_levels);
	if (TS == NULL) {
        fprintf(stderr, "error: malloc failure in mlfq_scheduling()\n");
        exit(1);
    }
	for( i=0; i<number_of_levels; i++ ){
		TS[i] = quantum * (i+1);
	}

	task_t* current_task; // pointer to the current task we are accessing
    int current_tick = 0;

	int next_task = 0; // checks for new tasks coming into system
	int tasks_completed = 0; // how many tasks have run til completion

	/*for (i = 0; i< num_tasks; i++) {
		printf("Arrival for task %d --- %d\n", i, tasks[i].arrival_time);
		printf("Priority for task %d --- %d\n", i, tasks[i].priority);
		printf("Length for task %d --- %f\n", i, tasks[i].length);
	}*/

    for (;;) {
		// check if next task is ready to run
		while (tasks[next_task].arrival_time <= current_tick) {
			if (next_task >= num_tasks) {
				break;
			}
			enqueue_task(Q[0], &tasks[next_task], &P[0], &N[0], size_of_queue);
			next_task+=1;
		}


		// grabbing the task to schedule
		// check sizes of queues going up in levels, until one has size > 0)
		for (i=0; i<number_of_levels; i++){
			if (N[i] > 0){
				break;
			}
		}
		// if no queues are of size > 0, then no tasks have arrived yet, go to next loop
		if (i>=number_of_levels){
			current_tick += 1;
			continue;
		}

		// i holds the level we are accessing
		// grab pointer to current task
		current_task = dequeue_task(Q[i], &P[i], &N[i], size_of_queue);
		current_task->schedulings += 1; // task has been scheduled
		
		// increment how long the task has ran for by time slice
        current_task->cpu_cycles += TS[i];
        current_tick += TS[i];

		// task is finished running
        if (current_task->cpu_cycles >= current_task->length) {
            float quantum_fragment = current_task->cpu_cycles -
                current_task->length;
            current_task->cpu_cycles = current_task->length;
            current_task->finish_time = current_tick - quantum_fragment;
			tasks_completed ++;
			
			// we are done all tasks
            if (tasks_completed >= num_tasks) {
                break;
            } 
			// the simulation pretends there is no downtime between different tasks
			current_tick -= quantum_fragment;
        }
		// if task is not finished, queue it on the next level
		else {
			int next_level;
			if( i < number_of_levels-1 ){
				next_level = i+1;
			} else { next_level = number_of_levels-1; }
			enqueue_task(Q[next_level], current_task, &P[next_level], &N[next_level], size_of_queue);
		}
    }
}


void run_simulation(int algorithm, int quantum)
{
    switch(algorithm) {
        case STRIDE:
            stride_scheduling(quantum);
            break;
        case PS:
            priority_scheduling();
            break;
        case MLFQ:
            mlfq_scheduling(quantum);
            break;
        case FCFS:
        default:
            first_come_first_serve();
            break;
    }
}


void compute_and_print_stats()
{
    int tasks_at_level[PRIORITY_LEVELS] = {0,};
    float response_at_level[PRIORITY_LEVELS] = {0.0, };
    int scheduling_events = 0;
    int i;

    for (i = 0; i < num_tasks; i++) {
        tasks_at_level[tasks[i].priority]++;
        response_at_level[tasks[i].priority] += 
            tasks[i].finish_time - (tasks[i].arrival_time * 1.0);
        scheduling_events += tasks[i].schedulings;

        printf("Task %3d: cpu time (%4.1f), response time (%4.1f), waiting (%4.1f), schedulings (%5d)\n",
            i, tasks[i].length,
            tasks[i].finish_time - tasks[i].arrival_time,
            tasks[i].finish_time - tasks[i].arrival_time - tasks[i].cpu_cycles,
            tasks[i].schedulings);
            
    }

    printf("\n");

    if (num_tasks > 0) {
        for (i = 0; i < PRIORITY_LEVELS; i++) {
            if (tasks_at_level[i] == 0) {
                response_at_level[i] = 0.0;
            } else {
                response_at_level[i] /= tasks_at_level[i];
            }
            printf("Priority level %d: average response time (%4.1f)\n",
                i, response_at_level[i]);
        }
    }

    printf ("Total number of scheduling events: %d\n", scheduling_events);
}


int main(int argc, char *argv[])
{
    int i = 0;
    int algorithm = FCFS;
    int quantum = 1;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            i++;
            quantum = atoi(argv[i]);
        } else if (strcmp(argv[i], "-a") == 0) {
            i++;
            if (strcmp(argv[i], "FCFS") == 0) {
                algorithm = FCFS;
            } else if (strcmp(argv[i], "PS") == 0) {
                algorithm = PS;
            } else if (strcmp(argv[i], "MLFQ") == 0) {
                algorithm = MLFQ;
            } else if (strcmp(argv[i], "STRIDE") == 0) {
                algorithm = STRIDE;
            }
        }
    }
         
    read_task_data();

    if (num_tasks == 0) {
        fprintf(stderr,"%s: no tasks for the simulation\n", argv[0]);
        exit(1);
    }

    init_simulation_data(algorithm);
    run_simulation(algorithm, quantum);
    compute_and_print_stats();

    exit(0);
}
