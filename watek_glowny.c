#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    int tag;
    int perc;

	MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;

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

	while(1){
		Enter(0, 1, pakiet, status);
		for (int i=0; i<number_of_workshops_per_participant; i++){
			Enter(my_workshops[i], 1, pakiet, status);
			Leave(my_workshops[i]);
		}
		Leave(0);
		wait_for_all();
	}

    // while (stan != InFinish) {
	// switch (stan) {
	//     case InRun: 
	// 	perc = random()%100;
	// 	if ( perc < 25 ) {
	// 	    debug("Perc: %d", perc);
	// 	    println("Ubiegam się o sekcję krytyczną")
	// 	    debug("Zmieniam stan na wysyłanie");
	// 	    packet_t *pkt = malloc(sizeof(packet_t));
	// 	    pkt->data = perc;
	// 	    ackCount = 0;
	// 	    for (int i=0;i<=size-1;i++)
	// 		if (i!=rank)
	// 		    sendPacket( pkt, i, REQUEST);
	// 	    changeState( InWant ); // w VI naciśnij ctrl-] na nazwie funkcji, ctrl+^ żeby wrócić
	// 				   // :w żeby zapisać, jeżeli narzeka że w pliku są zmiany
	// 				   // ewentualnie wciśnij ctrl+w ] (trzymasz ctrl i potem najpierw w, potem ]
	// 				   // między okienkami skaczesz ctrl+w i strzałki, albo ctrl+ww
	// 				   // okienko zamyka się :q
	// 				   // ZOB. regułę tags: w Makefile (naciśnij gf gdy kursor jest na nazwie pliku)
	// 	    free(pkt);
	// 	} // a skoro już jesteśmy przy komendach vi, najedź kursorem na } i wciśnij %  (niestety głupieje przy komentarzach :( )
	// 	debug("Skończyłem myśleć");
	// 	break;
	//     case InWant:
	// 	println("Czekam na wejście do sekcji krytycznej")
	// 	// tutaj zapewne jakiś semafor albo zmienna warunkowa
	// 	// bo aktywne czekanie jest BUE
	// 	if ( ackCount == size - 1) 
	// 	    changeState(InSection);
	// 	break;
	//     case InSection:
	// 	// tutaj zapewne jakiś muteks albo zmienna warunkowa
	// 	println("Jestem w sekcji krytycznej")
	// 	    sleep(5);
	// 	//if ( perc < 25 ) {
	// 	    debug("Perc: %d", perc);
	// 	    println("Wychodzę z sekcji krytycznej")
	// 	    debug("Zmieniam stan na wysyłanie");
	// 	    packet_t *pkt = malloc(sizeof(packet_t));
	// 	    pkt->data = perc;
	// 	    for (int i=0;i<=size-1;i++)
	// 		if (i!=rank)
	// 		    sendPacket( pkt, (rank+1)%size, RELEASE);
	// 	    changeState( InRun );
	// 	    free(pkt);
	// 	//}
	// 	break;
	//     default: 
	// 	break;
    //         }
    //     sleep(SEC_IN_STATE);
    // }
}


void Enter(int workshop_id, int number_of_tickets_for_requested, packet_t pakiet, MPI_Status status)
{
    zegar += 1;
    for (int i=0;i<=number_of_participants-1;i++){
        if (i!=rank){
            sendPacket( number_of_tickets, i, REQUEST, workshop_id);
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
        else if (status.MPI_TAG == REQUEST){ 
            if(workshop_id == pakiet.workshop_id){
                if (pakiet.ts < zegar || (pakiet.ts == zegar && pakiet.src < rank)){
                    sendPacket( 0, status.MPI_SOURCE, ACK, workshop_id );
                }
                else{
                    waiting_queue[workshop_id][indexes_for_waiting_queue[workshop_id]] = rank;
                    indexes_for_waiting_queue[workshop_id]++;
                    number_of_acks[rank]++;
                }
            }
            else{
                sendPacket( 0, status.MPI_SOURCE, ACK, pakiet.workshop_id );
            }
        }
        else if(status.MPI_TAG == RELEASE && workshop_id == pakiet.workshop_id){ 
            if(indexes_for_waiting_queue[workshop_id] > 0){
                sendPacket( 0, waiting_queue[workshop_id][0], ACK, workshop_id );
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
            sendPacket( 0, i, RELEASE, workshop_id);
        }
    }
    indexes_for_waiting_queue[workshop_id] = 0;
}

void wait_for_all(){
    while (finished < number_of_tickets){
        sleep(1);
    }
}
