#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <unistd.h>

#define PULSE_CODE   _PULSE_CODE_MINAVAIL

int global_time = 420;
int total_customer = 0;
int max_customer_queue_wait = 0;
int current_queue_legth = 0;
int max_length_queue = 0;
int max_transaction_time_1 = 0;
int max_transaction_time_2 = 0;
int max_transaction_time_3 = 0;
int max_transaction_time = 0;
int total_queue_time = 0;
int total_service_time = 0;

pthread_mutex_t lock;
pthread_mutex_t max_value_lock;

typedef union
{
        struct _pulse   pulse;
} my_message_t; 					  //This union is for timer module.

void *time_update( void *ptr );
void *teller_1( void *ptr );
void *teller_2( void *ptr );
void *teller_3( void *ptr );

struct Customer
 {
        int time_in;
        int time_out;
        struct Customer* next;
 }*rear, *front;

int customer_exit()
{
	  pthread_mutex_lock(&lock);
	  current_queue_legth--;
      struct Customer  *var=rear;

      if(var==rear)
      {
             rear = rear->next;
             int current_customer_queuetime = var->time_in-global_time;
             total_queue_time += (current_customer_queuetime);
             if (current_customer_queuetime > max_customer_queue_wait)
            	 max_customer_queue_wait = current_customer_queuetime;
             free(var);
             pthread_mutex_unlock(&lock);
             return 1;
      }
      else{
    	  printf("\nQueue Empty");
    	  pthread_mutex_unlock(&lock);
    	  return 0;
      }

}

void customer_entry(int value)
{
	 pthread_mutex_lock(&lock);
	 current_queue_legth++;
	 if(current_queue_legth > max_length_queue)
		 max_length_queue = current_queue_legth;
	 struct Customer *temp;
     temp=(struct Customer *)malloc(sizeof(struct Customer));
     temp->time_in=value;

     if (front == NULL)
     {
           front=temp;
           front->next=NULL;
           rear=front;
     }
     else
     {
           front->next=temp;
           front=temp;
           front->next=NULL;
     }
     pthread_mutex_unlock(&lock);
}

void display_queue_status()
{
	 pthread_mutex_lock(&lock);
     struct Customer *var=rear;
     if(var!=NULL)
     {
           printf("\n Customers in queue:  ");
           while(var!=NULL)
           {
                pthread_mutex_lock(&lock);
                printf("\t%d",var->time_in);
                pthread_mutex_unlock(&lock);
                var = var->next;
           }
     printf("\n");
     }
     else
     printf("Queue is Empty\n");
     pthread_mutex_unlock(&lock);
}


void main()
{
    printf("Bank is open.. Customer can come in now..:)\n");
	pthread_t thread0, thread1, thread2, thread3;
    const char *msg1 = "Teller 1";
    const char *msg2 = "Teller 2";
    const char *msg3 = "Teller 3";
    int next_customer_in_time;
    int  timerThread, tellerThread1, tellerThread2, tellerThread3;

    // This is mutex for saving queue from Parallel access.
    if (pthread_mutex_init(&lock, NULL) != 0)
        {
            printf("\n mutex init failed\n");
            return 1;
        }
    if (pthread_mutex_init(&max_value_lock, NULL) != 0)
        {
            printf("\n Mutex init failed for max_value_lock\n");
            return 1;
        }
    timerThread   = pthread_create( &thread0, NULL, time_update, (void*) msg1);				/* independent threads for tellers */
    tellerThread1 = pthread_create( &thread1, NULL, teller_1, (void*) msg1);
    tellerThread2 = pthread_create( &thread2, NULL, teller_2, (void*) msg2);
    tellerThread3 = pthread_create( &thread3, NULL, teller_3, (void*) msg3);

     while (global_time > 0)
     {
    	 srand(1);
    	 customer_entry(global_time);
    	 total_customer ++;

    	 next_customer_in_time = rand() % 1 + 1;
         usleep( 100000 * next_customer_in_time );

     }


     pthread_join( thread1, NULL);
     pthread_join( thread2, NULL);
     pthread_mutex_destroy(&max_value_lock);   				// This is destroying the mutex.
     pthread_mutex_destroy(&lock);

     printf("\n Bank is closed for the day.. Sorry for the inconvenience. \n");
     printf("Total number of customers serviced today : %d\n", total_customer);
     printf("Wait time for customer in queue= %d Min\n", max_customer_queue_wait);
     printf("Number of customers in the queue = %d customers\n", max_length_queue);
     printf("Max transaction time for teller-1 : %d\nMax transaction time for teller-2 : %d\nMax transaction time for teller-3 : %d\n", max_transaction_time_1, max_transaction_time_2, max_transaction_time_3);
     printf("Max transaction time for all the tellers : %d\n", max_transaction_time);
     printf("Avg time a customer spend in queue = %d min\n", total_queue_time/total_customer);
     printf("Avg time each customer spends with the teller = %d\n", total_service_time/total_customer);

     exit(0);
}

void *time_update( void *ptr )
{
	 struct sigevent         event;
	 struct itimerspec       itime;
	 timer_t                 timer_id;
	 int                     chid, rcvid;
	 my_message_t            msg;

	 chid = ChannelCreate(0);

	 event.sigev_notify = SIGEV_PULSE;
	 event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,chid,_NTO_SIDE_CHANNEL, 0);
	 event.sigev_priority = getprio(0);
	 event.sigev_code = PULSE_CODE;
	 timer_create(CLOCK_REALTIME, &event, &timer_id);

	 itime.it_value.tv_sec = 0;

	 itime.it_value.tv_nsec = 1000000;    /* 100 ms = .1 secs */
	 itime.it_interval.tv_sec = 0;		  /* 100 ms = .1 secs */

	 itime.it_interval.tv_nsec = 1000000;
	 timer_settime(timer_id, 0, &itime, NULL);

	 // This for loop will update the global_time for every 100 ms which is 1 minute in simulation time.

	 for (;;)
	 {
	     rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
	     if (rcvid == 0)
	     {
	          if (msg.pulse.code == PULSE_CODE) {

	        	  if (global_time == 360)
	        	       {
	        	       	printf("Time is 10:00\n");
	        	       }
	        	       else if (global_time == 300)
	        	       {
	        	           printf("Time is 11:00\n");
	        	        }
	        	       else if (global_time == 240)
	        	           {
	        	               printf("Time is 12:00\n");
	        	            }
	        	       else if (global_time == 180)
	        	           {
	        	               printf("Time is 1:00\n");
	        	           }
	        	       else if (global_time == 120)
	        	           {
	        	               printf("Time is 2:00\n");
	        	            }
	        	       else if (global_time == 60)
	        	           {
	        	               printf("Time is 3:00\n");
	        	            }
	        	       else if (global_time == 0)
	        	               {
	        	                   printf("Time is 4:00\n");
	        	                }
	        	  global_time--;
	          }
	     }
    }
}

void *teller_1( void *ptr )
{
	 usleep(400000);
     while(/*global_time > 0 || */front!=0 && rear != 0){
    	 if (customer_exit() == 0) {
    		 printf("Teller_1 -----> sleeping\n");
    		 usleep(50000);
    	 }
         else
         {

        	 	 srand(1);
        	 	 int customer_time;   			 //individual customer time.
        	 	 	 	 	 	 	 	 	 	 //customer_time = rand() % 12 + 1;
				 customer_time = rand() % 6 + 1;
	    		 pthread_mutex_lock(&max_value_lock);
	    		 total_service_time += customer_time;   //update the total service time for the Bank database.
	    		 pthread_mutex_unlock(&max_value_lock);

			 if(customer_time > max_transaction_time_1)
				 max_transaction_time_1 = customer_time;
			 if(customer_time > max_transaction_time){
				 pthread_mutex_lock(&max_value_lock);
				 max_transaction_time = customer_time;   //update the max transaction time for the Bank database.
				 pthread_mutex_unlock(&max_value_lock);
			 }
			 	 	 	 	 	 	 	 	 	 	 	 //printf("teller_1 : global_time = %d and customer_time = %d min\n", global_time, ((customer_time / 2) + (customer_time %2)) );
			 usleep( 50000 * customer_time );
         }

     }
     printf("Teller_1 -----> offline.\n");
}

void *teller_2( void *ptr )
{
	 usleep(700000);
     while(front!=0 && rear != 0){
    	 if (customer_exit() == 0) {
    	     		 printf("Teller_1 -----> sleeping\n");
    	     		 usleep(50000);
    	 }
    	 else {
    		 srand(1);
    		 int customer_time;   						  	 	//individual customer time.
    		 customer_time = rand() % 6 + 1;

    		 pthread_mutex_lock(&max_value_lock);
    		 total_service_time += customer_time; 				//update the total service time for the Bank database.
    		 pthread_mutex_unlock(&max_value_lock);

    		 if(customer_time > max_transaction_time_2)
    			 max_transaction_time_2 = customer_time;
    		 if(customer_time > max_transaction_time){
    			 pthread_mutex_lock(&max_value_lock);
    			 max_transaction_time = customer_time;   		//update the max transaction time for the Bank database.
    			 pthread_mutex_unlock(&max_value_lock);
    		 }
    		 usleep( 50000 * customer_time);
    	 }
     }
     printf("Teller_2 ----->  offline.\n");
}

void *teller_3( void *ptr )
{
     usleep(600000);
     while(/*global_time > 0 || */front!=0 && rear != 0){
    	 if (customer_exit() == 0) {
    	     		 printf("Teller_1 -----> sleeping\n");
    	     		 usleep(50000);
    	 }
    	 else{
    		 srand(1);
    		 int customer_time;     //individual customer time.
    		 customer_time = rand() % 6 + 1;
    		 pthread_mutex_lock(&max_value_lock);
    		 total_service_time += customer_time;  //update the total service time for the Bank database.
    		 pthread_mutex_unlock(&max_value_lock);
    		 if(customer_time > max_transaction_time_3)
    			 max_transaction_time_3 = customer_time;
    		 if(customer_time > max_transaction_time){
    			 pthread_mutex_lock(&max_value_lock);
    			 max_transaction_time = customer_time;    //update the max transaction time for the Bank database.
    			 pthread_mutex_unlock(&max_value_lock);
    		 }
    		 usleep( 50000 * customer_time );
    	 }
     }

     printf("Teller_3 -----> offline.\n");
}

