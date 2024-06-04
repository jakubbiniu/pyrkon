#include "main.h"
#include "watek_glowny.h"

void reset_variables() {
    for (int i = 0; i < number_of_participants; i++) {
		finished[i] = 0;
        number_of_acks[i] = 0;
        workshop_count[i] = 0;
        on_pyrkon[i] = 0;
        for (int j = 0; j < number_of_people_per_workshop+1; j++) {
            my_workshops[i][j] = 0;
        }
    }
    for (int i = 0; i < number_of_workshops + 1; i++) {
        indexes_for_waiting_queue[i] = 0;
        for (int j = 0; j < number_of_participants; j++) {
            waiting_queue[i][j] = 0;
        }
    }
}



void mainLoop()
{
    srandom(rank);
    int tag;
    int perc;

	int previous_workshop_id;

	MPI_Status status;
    // int is_message = FALSE;
    packet_t pakiet;
  
	packet_t *pkt = malloc(sizeof(packet_t));
    while (stan != InFinish) {
		workshop_id = my_workshops[rank][workshop_count[rank]];
	switch (stan) {
		case BeginPyrkon:
		printf("heja %d\n", number_of_workshops_per_participant);
		my_workshops[rank][0] = 0;
		for (int i=1;i<=number_of_workshops_per_participant;i++){
			int candidate = random()%number_of_workshops + 1;
			for(int j=0;j<i;j++){
				if (my_workshops[rank][j] == candidate){
					candidate = random()%number_of_workshops + 1;
					j = 0;
				}
			}
			printf("dziko %d %d\n", rank, i);
			my_workshops[rank][i] = candidate;
		}
				printf("heja \n");

		for (int i=0;i<=number_of_workshops_per_participant;i++){
			println("%d: Mój warsztat %d\n",rank ,my_workshops[rank][i]);
		}
		printf("heja2\n");
		fflush(0);
		changeState(InRun);

	    case InRun: 
		perc = random()%100;
		if ( perc < 25 ) {
		    debug("Perc: %d", perc);
			if(workshop_count[rank] == 0){
				println("Chcę wejść na pyrkon")
			}
			else{
				println("Chcę wejść na warsztat %d", workshop_id)
			}
		    debug("Zmieniam stan na wysyłanie");
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
			pkt->workshop_id = my_workshops[rank][workshop_count[rank]];
		    number_of_acks[rank] = 0;
		    for (int i=0;i<=size-1;i++)
			if (i!=rank){
			    sendPacket(pkt, i, REQUEST, pkt->workshop_id);
				if(pkt->workshop_id == 0){
					// println("Wysyłam request na pyrkon do %d", i)
				}
				else{
					// println("Wysyłam request na warsztat %d do %d", pkt->workshop_id, i)
				}
			}
			if(workshop_count[rank] == 0){
				changeState(InWantPyrkon);
			}
			else{
				changeState(InWant);
			}
		    // changeState( InWant ); // w VI naciśnij ctrl-] na nazwie funkcji, ctrl+^ żeby wrócić
					   // :w żeby zapisać, jeżeli narzeka że w pliku są zmiany
					   // ewentualnie wciśnij ctrl+w ] (trzymasz ctrl i potem najpierw w, potem ]
					   // między okienkami skaczesz ctrl+w i strzałki, albo ctrl+ww
					   // okienko zamyka się :q
					   // ZOB. regułę tags: w Makefile (naciśnij gf gdy kursor jest na nazwie pliku)
		    free(pkt);
		} // a skoro już jesteśmy przy komendach vi, najedź kursorem na } i wciśnij %  (niestety głupieje przy komentarzach :( )
		debug("Skończyłem myśleć");
		break;

	    case InWantPyrkon:
		// println("mój ack count: %d Czekam na wejście na pyrkon", number_of_acks[rank])
		// tutaj zapewne jakiś semafor albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if (number_of_acks[rank] >= number_of_participants - number_of_tickets){
			workshop_count[rank] += 1;
			number_of_acks[rank] = 0;
			println("Jestem na pyrkonie")
			on_pyrkon[rank] = 1;
			for(int i=0;i<indexes_for_waiting_queue[0];i++){
				// println("w kolejce na pyrkonie %d", waiting_queue[0][i])
			}	
		    changeState(InRun);
		} 
		break;
		case InWant:
		println("mój ack_count %d Czekam na wejście na warsztat %d", number_of_acks[rank],workshop_id)
		// tutaj zapewne jakiś semafor albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if (number_of_acks[rank] >= number_of_participants - number_of_people_per_workshop){
			println("Jestem na warsztacie %d", workshop_id)
			number_of_acks[rank] = 0;
			workshop_count[rank] += 1;
		    changeState(InSection);
		}
		break;
		case InSection:
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		    sleep(5);
		//if ( perc < 25 ) {
		    debug("Perc: %d", perc);
			previous_workshop_id = my_workshops[rank][workshop_count[rank]-1];
		    println("Wychodzę z warsztatu %d", previous_workshop_id)
		    debug("Zmieniam stan na wysyłanie");
		    
		    pkt->data = perc;
			// pthread_mutex_lock(&zegarMut);
		    // zegar += 1;
			// pthread_mutex_unlock(&zegarMut);
			for (int i=0;i<=number_of_participants-1;i++){
				if (i!=rank){
					sendPacket( 0, i, RELEASE, previous_workshop_id);
				}
			}
			for (int i=0;i<indexes_for_waiting_queue[previous_workshop_id];i++){
				// println("wysylam ACK na warsztat %d do %d", previous_workshop_id,i);
				sendPacket( 0, waiting_queue[previous_workshop_id][i], ACK, previous_workshop_id);
			}
			indexes_for_waiting_queue[previous_workshop_id] = 0;
			if (workshop_count[rank] > number_of_workshops_per_participant){
				println("Wychodzę z pyrkonu")
				on_pyrkon[rank] = 0;
				debug("Zmieniam stan na wysyłanie");
				for (int i=0;i<=number_of_participants-1;i++){
					if (i!=rank){
						sendPacket( 0, i, FINISH, 0);
					}
				}
				for (int i=0;i<indexes_for_waiting_queue[0];i++){
					// println("wysylam ACK na pyrkon do %d", i);
					sendPacket( 0, waiting_queue[workshop_id][i], ACK, 0);
				}
				indexes_for_waiting_queue[0] = 0;
				changeState(FinishedWorkshops);
			}
			else{
				changeState( InRun );
			}
		    // free(pkt);
		break;
		case FinishedWorkshops:
			println("Koniec pyrkonu dla mnie")
			while(finished[rank] < number_of_participants-1){

			}
			//MPI_Barrier(MPI_COMM_WORLD);
			//if(rank==1){
				reset_variables();
			//}
			println("KONIEC PYRKONU!!!")
			/*for (int i = 0; i < number_of_participants; i++) {
				println(" moj ack %d",number_of_acks[i]);
				println(" moj workshop_count %d",workshop_count[i]);
				println(" moj on_pyrkon %d",on_pyrkon[i]);
				for (int j = 0; j < number_of_people_per_workshop+1; j++) {
					println(" moj my_workshops %d",my_workshops[i][j]);
				}
			}*/
			changeState(BeginPyrkon);
		break;
	    default: 
		break;
        }
        sleep(SEC_IN_STATE);
    }
}
