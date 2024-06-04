/* w main.h także makra println oraz debug -  z kolorkami! */
#include "main.h"
#include "watek_glowny.h"
#include "watek_komunikacyjny.h"

/*
 * W main.h extern int rank (zapowiedź) w main.c int rank (definicja)
 * Zwróćcie uwagę, że każdy proces ma osobą pamięć, ale w ramach jednego
 * procesu wątki współdzielą zmienne - więc dostęp do nich powinien
 * być obwarowany muteksami. Rank i size akurat są write-once, więc nie trzeba,
 * ale zob util.c oraz util.h - zmienną state_t state i funkcję changeState
 *
 */
int rank, size;
int workshop_id;
/* 
 * Każdy proces ma dwa wątki - główny i komunikacyjny
 * w plikach, odpowiednio, watek_glowny.c oraz (siurpryza) watek_komunikacyjny.c
 *
 *
 */

pthread_t threadKom;

int zegar=0; // zegar lamporta
int number_of_tickets=0; // liczba biletów na pyrkon
int number_of_workshops=0;  // liczba warsztatów
int number_of_people_per_workshop = 0; // liczba uczestników na warsztat
int number_of_participants = 0; // liczba uczestników
int number_of_workshops_per_participant=0; // liczba warsztatów na uczestnika
int *number_of_acks; // dla kazdego uczestnika liczymy liczbe acks
int **waiting_queue; // kolejka oczekujących na bilet uczestników dla kazdego warsztatu + dla pyrkonu
int *indexes_for_waiting_queue; // indeksy dla kolejki oczekujących na bilet dla każdego warsztatu + dla pyrkonu
int *workshop_count; // dla każdego uczestnika liczymy liczbę warsztatów, na których był licząc też pyrkon jako jeden warsztat
int **my_workshops; // dla każdego uczestnika zapisujemy listę warsztatów, na które się zapisał (zacyznamy od 0 - pyrkonu) 
int *on_pyrkon; // dla każdego uczestnika zapisujemy czy jest na pyrkonie
int ***local_request_ts;
int *finished;

void finalizuj()
{
    pthread_mutex_destroy( &stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadKom,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
    free_arrays();
}

void free_arrays(){
    free(number_of_acks);
    for (int i = 0; i < number_of_workshops + 1; ++i) {
        free(waiting_queue[i]);
    }
    free(waiting_queue);
    free(indexes_for_waiting_queue);
    free(workshop_count);
    for (int i = 0; i < number_of_participants; ++i) {
        free(my_workshops[i]);
    }
    free(my_workshops);
    free(on_pyrkon);
    for (int i = 0; i < number_of_participants; ++i) {
        for (int j = 0; j < number_of_workshops + 1; ++j) {
            free(local_request_ts[i][j]);
        }
        free(local_request_ts[i]);
    }
    free(local_request_ts);
    free(finished);
}

void initialize_arrays() {
    number_of_acks = (int *)malloc(number_of_participants * sizeof(int));
    waiting_queue = (int **)malloc((number_of_workshops + 1) * sizeof(int *));
    for (int i = 0; i < number_of_workshops + 1; ++i) {
        waiting_queue[i] = (int *)malloc(number_of_participants * sizeof(int));
    }
    indexes_for_waiting_queue = (int *)malloc((number_of_workshops + 1) * sizeof(int));
    workshop_count = (int *)malloc(number_of_participants * sizeof(int));
    my_workshops = (int **)malloc(number_of_participants * sizeof(int *));
    for (int i = 0; i < number_of_participants; ++i) {
        my_workshops[i] = (int *)malloc((number_of_workshops + 1) * sizeof(int));
    }
    on_pyrkon = (int *)malloc(number_of_participants * sizeof(int));
    local_request_ts = (int ***)malloc(number_of_participants * sizeof(int **));
    for (int i = 0; i < number_of_participants; ++i) {
        local_request_ts[i] = (int **)malloc((number_of_workshops + 1) * sizeof(int *));
        for (int j = 0; j < number_of_workshops + 1; ++j) {
            local_request_ts[i][j] = (int *)malloc(number_of_participants * sizeof(int));
        }
    }
    finished = (int *)malloc(number_of_participants * sizeof(int));
}

void print_usage(const char *program_name) {
    printf("Użyj: %s <tickets> <workshops> <people_per_workshop> <participants> <workshops_per_participant>\n", program_name);
}

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}


int main(int argc, char **argv)
{
     if (argc != 6) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    number_of_tickets = atoi(argv[1]);
    number_of_workshops = atoi(argv[2]);
    number_of_people_per_workshop = atoi(argv[3]);
    number_of_participants = atoi(argv[4]);
    number_of_workshops_per_participant = atoi(argv[5]);


    initialize_arrays();


    MPI_Status status;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    srand(rank);
    /* zob. util.c oraz util.h */
    inicjuj_typ_pakietu(); // tworzy typ pakietu
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /* startKomWatek w watek_komunikacyjny.c 
     * w vi najedź kursorem na nazwę pliku i wciśnij klawisze gf
     * powrót po wciśnięciu ctrl+6
     * */
    pthread_create( &threadKom, NULL, startKomWatek , 0);

    /* mainLoop w watek_glowny.c 
     * w vi najedź kursorem na nazwę pliku i wciśnij klawisze gf
     * powrót po wciśnięciu ctrl+6
     * */
    mainLoop(); // możesz także wcisnąć ctrl-] na nazwie funkcji
		// działa, bo używamy ctags (zob Makefile)
		// jak nie działa, wpisz set tags=./tags :)
    
    finalizuj();
    return 0;
}

