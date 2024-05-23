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

	packet_t *pkt = malloc(sizeof(packet_t));
    while (stan != InFinish) {
		workshop_id = my_workshops[rank][workshop_count[rank]];
	switch (stan) {
		case BeginPyrkon:
		my_workshops[rank][0] = 0;
		for (int i=1;i<=number_of_workshops_per_participant;i++){
			int candidate = random()%number_of_workshops + 1;
			for(int j=0;j<i;j++){
				if (my_workshops[rank][j] == candidate){
					candidate = random()%number_of_workshops;
					j = 0;
				}
			}
			my_workshops[rank][i] = candidate;
		}
		for (int i=0;i<=number_of_workshops_per_participant;i++){
			println("%d: Mój warsztat %d\n",rank ,my_workshops[rank][i]);
		}
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
					println("Wysyłam request na pyrkon do %d", i)
				}
				else{
					println("Wysyłam request na warsztat %d do %d", pkt->workshop_id, i)
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
		println("mój ack count: %d Czekam na wejście na pyrkon", number_of_acks[rank])
		// tutaj zapewne jakiś semafor albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if (number_of_acks[rank] >= number_of_participants - number_of_tickets){
			workshop_count[rank] += 1;
			number_of_acks[rank] = 0;
			println("Jestem na pyrkonie")
			on_pyrkon[rank] = 1;
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
		    println("Wychodzę z warsztatu %d", workshop_id)
		    debug("Zmieniam stan na wysyłanie");
		    
		    pkt->data = perc;
		    zegar += 1;
			for (int i=0;i<=number_of_participants-1;i++){
				if (i!=rank){
					sendPacket( 0, i, RELEASE, workshop_id);
				}
			}
			for (int i=0;i<indexes_for_waiting_queue[workshop_id];i++){
				sendPacket( 0, waiting_queue[workshop_id][i], ACK, workshop_id);
			}
			indexes_for_waiting_queue[workshop_id] = 0;
			if (workshop_count[rank] == number_of_workshops_per_participant){
				println("Wychodzę z pyrkonu")
				on_pyrkon[rank] = 0;
				debug("Zmieniam stan na wysyłanie");
				pkt->data = perc;
				zegar += 1;
				for (int i=0;i<=number_of_participants-1;i++){
					if (i!=rank){
						sendPacket( 0, i, RELEASE, 0);
					}
				}
				for (int i=0;i<indexes_for_waiting_queue[0];i++){
					sendPacket( 0, waiting_queue[workshop_id][i], ACK, 0);
				}
				indexes_for_waiting_queue[0] = 0;
				changeState(FinishedWorkshops);
			}
			else{
				changeState( InRun );
			}
		    free(pkt);

		break;
		case FinishedWorkshops:
		println("Koniec warsztatów")
		finished++;
		while(finished < number_of_tickets){
			sleep(1);
		}
		changeState(BeginPyrkon);
	    default: 
		break;
        }
        sleep(SEC_IN_STATE);
    }
}
