#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


//Estructuras
typedef struct{

	int ID;
	int Atendido; //
	int Tipo; //Edad de los pacientes, 0-15-junior, 16-59-medio, 60-100 senior 
	int Serologia; // 0-Participan, 1-No

} Pacientes;

typedef struct{

	int ID;
	int Tipo; //0-
	int Ocupado; //0-Ocupado, 1-Libre
	pthread_t enfermero; 

}Enfermero;

//Variables

int contadorPacientes, terminado;
pthread_mutex_t mutexLog, mutexPacientes, mutexPacientesEstudio;

Pacientes colaPacientes[5];

//hilos
pthread_t medico, estadistico;

FILE *logFile;

void nuevoPaciente(int signal);
void accionesPaciente();
void *accionesEnfermero(void *arg);
void *accionesMedico(void *arg);
void *accionesEstadistico(void *arg);
void finalizar(int signal);


int main(int argc, char *argv[]){

	struct sigaction pacienteJunior, pacienteMedio, pacienteSenior, terminar;

	pacienteJunior.sa_handler = nuevoPaciente;
	pacienteMedio.sa_handler = nuevoPaciente;
	pacienteSenior.sa_handler = nuevoPaciente;
	
	terminar.sa_handler = finalizar;

	sigaction(SIGUSR1, &pacienteJunior, NULL);	//Manejo SIGUSR1 para los pacientes junior
        sigaction(SIGUSR2, &pacienteMedio, NULL);	//Manejo SIGUSR2 para los pacientes Medios
	sigaction(SIGPIPE, &pacienteSenior, NULL);	//Manejo SIGPIPE para los pacientes Senior	
        sigaction(SIGINT, &terminar, NULL);		//Manejo SIGINT para terminar

	//Inicializar Recursos
	pthread_mutex_init(&mutexLog, NULL);
	pthread_mutex_init(&mutexPacientes, NULL);
	pthread_mutex_init(&mutexPacientesEstudio, NULL);

	contadorPacientes = 0;
	terminado = 0;

	//Inicializar estructuras a 0
	int i;	
	for(i=0; i<5; i++)//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE a numero leido
	{
	
		colaPacientes[i].ID = 0;
		colaPacientes[i].Atendido = 0;
		colaPacientes[i].Tipo = 0;
		colaPacientes[i].Serologia = 0;

	}

	//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE ENFERMEROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOS

	logFile = fopen("registroTiempos.log", "w");

	//Creacion de hilos
	//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE ENFERMEROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOS
	pthread_create(&medico, NULL, &accionesMedico, NULL);
	pthread_create(&estadistico, NULL, &accionesEstadistico, NULL);

	 while(terminado == 0){
                pause;
        }

}

