#include "main.h"
#include "watek_komunikacyjny.h"


/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {

        while(1){
            Enter(0, number_of_tickets);
            printf("Wchodzę na pyrkon\n");
            int my_workshops[number_of_workshops_per_participant];
            for (int i=0;i<number_of_workshops_per_participant;i++){
                int candidate = random()%number_of_workshops;
                for(int j=0;j<i;j++){
                    if (my_workshops[j] == candidate){
                        candidate = random()%number_of_workshops;
                        j = 0;
                    }
                }
                my_workshops[i] = candidate;
            }
            for (int i=0;i<number_of_workshops_per_participant;i++){
                printf("Mój warsztat %d\n", my_workshops[i]);
            }
            for (int i=0;i<number_of_workshops_per_participant;i++){
                Enter(my_workshops[i], number_of_people_per_workshop);
                printf("Wchodzę na warsztat %d\n", my_workshops[i]);
                Leave(my_workshops[i]);
            }
            finished++;
            wait_for_all(); // do zaimplementowania       
        }
	// debug("czekam na recv");
    //     MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    //     pthread_mutex_lock(&zegarMut);
    //     if(zegar >= pakiet.ts){
    //         zegar++;
    //     }
    //     else{
    //         zegar = pakiet.ts + 1;
    //     }
    //     pthread_mutex_unlock(&zegarMut);

    //     switch ( status.MPI_TAG ) {
	//     case REQUEST: 
    //             debug("Ktoś coś prosi. A niech ma!")
	// 	sendPacket( 0, status.MPI_SOURCE, ACK );
	//     break;
	//     case ACK: 
    //             debug("Dostałem ACK od %d, mam już %d", status.MPI_SOURCE, ackCount);
	//         ackCount++; /* czy potrzeba tutaj muteksa? Będzie wyścig, czy nie będzie? Zastanówcie się. */
	//     break;
	//     default:
	//     break;
    //     }
    }
}

void Enter(int workshop_id, int number_of_tickets_for_requested)
{
    zegar += 1;
    for (int i=0;i<=number_of_participants-1;i++){
        if (i!=rank){
            sendPacket( number_of_tickets, i, REQUEST);
        }
    }
    number_of_acks[rank] = 0;
    while (number_of_acks[rank] < number_of_participants - number_of_tickets_for_requested){
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&zegarMut);
        if(zegar >= pakiet.ts){
            zegar++;
        }
        else{
            zegar = pakiet.ts + 1;
        }
        pthread_mutex_unlock(&zegarMut);
        if (status.MPI_TAG == ACK && workshop_id == pakiet.workshop_id){ 
            number_of_acks[rank]++;
        }
        elif (status.MPI_TAG == REQUEST){ 
            if(workshop_id = pakiet.workshop_id){
                if (pakiet.ts < zegar || (pakiet.ts == zegar && pakiet.rank < rank)){
                    sendPacket( 0, status.MPI_SOURCE, ACK );
                }
                else{
                    waiting_queue[workshop_id][indexes_for_waiting_queue[workshop_id]] = rank;
                    indexes_for_waiting_queue[workshop_id]++;
                    number_of_acks[rank]++;
                }
            }
            else{
                sendPacket( 0, status.MPI_SOURCE, ACK );
            }
        }
        elif(status.MPI_TAG == RELEASE && workshop_id == pakiet.workshop_id){ 
            if(indexes_for_waiting_queue[workshop_id] > 0){
                sendPacket( 0, waiting_queue[workshop_id][0], ACK );
                for(int i=0; i<indexes_for_waiting_queue[workshop_id]-1; i++){
                    waiting_queue[workshop_id][i] = waiting_queue[workshop_id][i+1];
                }
                indexes_for_waiting_queue[workshop_id]--;
            }
        }
    }
}

void Leave(int workshop_id){
    zegar += 1;
    for (int i=0;i<=number_of_participants-1;i++){
        if (i!=rank){
            sendPacket( 0, i, RELEASE);
        }
    }
    indexes_for_waiting_queue[workshop_id] = 0;
}

void wait_for_all(){
    while (finished < number_of_tickets){
        sleep(1);
    }
}


