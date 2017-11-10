#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define DEBUG 0

typedef enum {LEFT=0, RIGHT=1} Side;

// Global semaphores
sem_t rope_mutex[1];
sem_t rope_slots[1];
sem_t starvation_preventer[1];

// Number in queue
unsigned int queue[2];
sem_t queue_mutex[2];

// Pthread attr
pthread_attr_t attr[1];

unsigned int cross_time;

// Baboon structs
typedef struct _Baboon{
  struct _Baboon* next;
  Side side;
  unsigned int number;
  pthread_t thread;
} Baboon;

typedef struct {
  Baboon* head;
  Baboon* tail;
  unsigned int length;
} BaboonList;

// Function forward declarion
void* baboon_cross(void* args);

void create_baboon(BaboonList* list, unsigned int number, Side side) {
  Baboon* b = calloc(1, sizeof(Baboon));
  b->side = side;
  b->next = NULL;
  b->number = number;

  // Add to list
  if (!list->head) {
    list->head = b;
    list->tail = b;
  } else {
    list->tail->next = b;
    list->tail = b;
  }

  // Start crossing
  pthread_create(&b->thread, attr, baboon_cross, (void*)b);
  if (DEBUG) printf("Baboon %d created with side: %d\n", b->number, side);
}

void* baboon_cross(void* args) {
  Baboon* b = (Baboon*)args;

  if (DEBUG) printf("Baboon %d waiting for starvation preventer\n", b->number);
  fflush(stdout);
  sem_wait(starvation_preventer);

  // Increment queue
  if (DEBUG) printf("Baboon %d waiting for queue mutex\n", b->number);
  fflush(stdout);
  sem_wait(&queue_mutex[b->side]);
  queue[b->side]++;
  if (queue[b->side] == 1) {
    if (DEBUG) printf("Baboon %d waiting for rope mutex\n", b->number);
    fflush(stdout);
    sem_wait(rope_mutex);
  }
  sem_post(&queue_mutex[b->side]);

  sem_post(starvation_preventer);

  // Wait for rope slot
  if (DEBUG) printf("Baboon %d waiting for rope slot\n", b->number);
  fflush(stdout);
  sem_wait(rope_slots);

  // Monkey crosses
  if (DEBUG) printf("Baboon %d crossing\n", b->number);
  fflush(stdout);
  sleep(cross_time);
  printf("Baboon %d crossed\n", b->number);
  fflush(stdout);

  // Release rope slot
  sem_post(rope_slots);


  // Decrement queue
  if (DEBUG) printf("Baboon %d waiting for queue mutex\n", b->number);
  fflush(stdout);
  sem_wait(&queue_mutex[b->side]);
  queue[b->side]--;
  if (!queue[b->side]) {
    sem_post(rope_mutex);
  }
  sem_post(&queue_mutex[b->side]);

  return NULL;
} 

int main(int argc, char* argv[]) {
  if (argc != 3) {
    puts("Usage: baboonsync <input file> <cross time>");
    exit(1);
  }
  cross_time = atoi(argv[2]);

  // Initialize semaphores
  sem_init(rope_slots, 0, 3);
  sem_init(rope_mutex, 0, 1);
  sem_init(&queue_mutex[0], 0, 1);
  sem_init(&queue_mutex[1], 0, 1);
  sem_init(starvation_preventer, 0, 1);

  // Initialize counts
  queue[LEFT] = queue[RIGHT] = 0;

  // Initialize pthread attr
  pthread_attr_init(attr);
  pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);

  // Create BaboonList
  BaboonList* baboon_list = calloc(1, sizeof(BaboonList));

  // Read file and create baboons
  unsigned int number = 0;
  FILE* f = fopen(argv[1], "r");
  char c;
  while(fscanf(f, "%c", &c) != EOF) {

    switch (c) {
      case 'L':
        create_baboon(baboon_list, number++, LEFT);
        break;
      case 'R':
        create_baboon(baboon_list, number++, RIGHT);
        break;
      case ',':
        continue;
      case '\n':
        continue;
      default:
        fputs("Illegal character encountered\n", stderr);
    }
  }

  // Wait for all baboons to cross
  for (Baboon* b = baboon_list->head; b != NULL; b = b->next) {
    pthread_join(b->thread, NULL);
  }

  // Destroy semaphores
  sem_destroy(rope_slots);
  sem_destroy(rope_mutex);
  sem_destroy(&queue_mutex[0]);
  sem_destroy(&queue_mutex[1]);
  sem_destroy(starvation_preventer);

  // Free the baboons
  for (Baboon* b = baboon_list->head; b != NULL;) {
    Baboon* cur = b;
    b = b->next;
    free(cur);
  }
  free(baboon_list);

}
  
