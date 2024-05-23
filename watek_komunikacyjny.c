#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
        // printf("Wątek komunikacyjny startuje %d\n", rank);
    
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {

        packet_t pakiet;
        MPI_Status status;

	    debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        pthread_mutex_lock(&zegarMut);
        if(zegar >= pakiet.ts){
            zegar++;
        }
        else{
            zegar = pakiet.ts + 1;
        }
        pthread_mutex_unlock(&zegarMut);

        workshop_id = my_workshops[rank][workshop_count[rank]];
        println("ubiegam sie o: %d", workshop_id);  
        if (status.MPI_TAG == ACK && pakiet.workshop_id == workshop_id){
            if(workshop_id !=0){
                println("Dostałem ACK od %d na warsztat %d", status.MPI_SOURCE, workshop_id);
            }
            else{
                println("Dostałem ACK od %d na pyrkon", status.MPI_SOURCE);
            }
            number_of_acks[rank] += 1;
        }
        else if (status.MPI_TAG == REQUEST){
            if(pakiet.workshop_id !=0){
                println("Dostałem REQUEST od %d na warsztat %d", status.MPI_SOURCE, pakiet.workshop_id);
            }
            else{
                println("Dostałem ACK od %d na pyrkon", status.MPI_SOURCE);
            }
            pthread_mutex_lock(&zegarMut);
            if(zegar >= pakiet.ts){
                zegar++;
            }
            else{
                zegar = pakiet.ts + 1;
            }
            pthread_mutex_unlock(&zegarMut);
            if(workshop_id == pakiet.workshop_id){
                if (pakiet.ts < zegar || (pakiet.ts == zegar && pakiet.src < rank)){
                    sendPacket( 0, status.MPI_SOURCE, ACK, workshop_id);
                    println("Wysyłam ACK do %d na warsztat %d", status.MPI_SOURCE, workshop_id);
                }
                else{
                    waiting_queue[workshop_id][indexes_for_waiting_queue[workshop_id]] = rank;
                    indexes_for_waiting_queue[workshop_id] += 1;
                    number_of_acks[rank] += 1;
                    println("Dodaję %d do kolejki oczekujących na warsztat %d", status.MPI_SOURCE, pakiet.workshop_id);
                }
            }
            else{
                if(pakiet.workshop_id != 0 || on_pyrkon[rank] == 0){
                    sendPacket( 0, status.MPI_SOURCE, ACK, pakiet.workshop_id);
                    println("Wysyłam ACK do %d na warsztat %d", status.MPI_SOURCE, pakiet.workshop_id);
                }
                else{
                    println("Nie mogę wysłać ACK do %d na warsztat %d", status.MPI_SOURCE, pakiet.workshop_id);
                }
            }
        }
        else if(status.MPI_TAG == RELEASE && pakiet.workshop_id == workshop_id){
            if(indexes_for_waiting_queue[workshop_id] > 0){
                sendPacket( 0, waiting_queue[workshop_id][0], ACK, workshop_id );
                for(int i=0; i<indexes_for_waiting_queue[workshop_id]-1; i++){
                    waiting_queue[workshop_id][i] = waiting_queue[workshop_id][i+1];
                }
                indexes_for_waiting_queue[workshop_id] -= 1;
            }
        }
    }
}



