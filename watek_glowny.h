#ifndef WATEK_GLOWNY_H
#define WATEK_GLOWNY_H

void Enter(int workshop_id, int number_of_tickets_for_requested, packet_t pakiet, MPI_Status status);
void Leave(int workshop_id);
void wait_for_all();


/* pętla główna aplikacji: zmiany stanów itd */
void mainLoop();

#endif
