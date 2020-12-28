#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>


//Estructuras
typedef struct{

	int ID;
	int Atendido; //0 - no, 1 - si
	int Tipo; //Edad de los pacientes, 0-15-junior, 16-59-medio, 60-100 senior, 0 - junior, 1 - medio, 2 - senior
	int Serologia; // 0-Participan, 1-No
	pthread_t paciente;

} Pacientes;

typedef struct{

	int ID;
	int Tipo; //0-
	int Ocupado; //0-Ocupado, 1-Libre
	pthread_t enfermero; 

}Enfermero;

//Variables

int contadorPacientes, terminado, nPacientes;
pthread_mutex_t mutexLog, mutexColaPacientes, mutexPacientesEstudio;

Pacientes colaPacientes[5];

//hilos
pthread_t medico, estadistico;

FILE *logFile;

void nuevoPaciente(int signal);
void *accionesPaciente(void *arg);
void *accionesEnfermero(void *arg);
void *accionesMedico(void *arg);
void *accionesEstadistico(void *arg);
void finalizar(int signal);
void writeLogMessage(char *identifier, int id, char *msg);


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
	pthread_mutex_init(&mutexColaPacientes, NULL);
	pthread_mutex_init(&mutexPacientesEstudio, NULL);

	contadorPacientes = 0;
	terminado = 0;
	nPacientes = 5;

	//Inicializar estructuras a 0
	int i;	
	for(i=0; i<nPacientes; i++)//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE a numero leido
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

void nuevoPaciente(int signal){

	pthread_mutex_lock(&mutexColaPacientes);

	int i = 0;
	bool anyadido = false;

	while(i<=nPacientes && anyadido == false){

		if(colaPacientes[i].ID == 0){
		
			contadorPacientes += 1;
	
			colaPacientes[i].ID = contadorPacientes;
			colaPacientes[i].Atendido = 0;
			
			//Comprobamos la senyal recibida para saber que tipo de paciente es
			if(signal == SIGUSR1){//El paciente es junior

				colaPacientes[i].Tipo = 0;

				pthread_mutex_lock(&mutexLog);
                                writeLogMessage("Sistema", 1, "Paciente anyadido de tipo junior");
                                pthread_mutex_unlock(&mutexLog);	

			}else if(signal == SIGUSR2){//El paciente es medio

				colaPacientes[i].Tipo = 1;			

				pthread_mutex_lock(&mutexLog);
                                writeLogMessage("Sistema", 1, "Paciente anyadido de tipo medio");
                                pthread_mutex_unlock(&mutexLog);

			}else if(signal = SIGPIPE){//El paciente es senior

				colaPacientes[i].Tipo = 2;

				pthread_mutex_lock(&mutexLog);
                                writeLogMessage("Sistema", 1, "Paciente anyadido de tipo senior");
                                pthread_mutex_unlock(&mutexLog);

			}

			colaPacientes[i].Serologia = 0;

			pthread_create(&colaPacientes[i].paciente, NULL, accionesPaciente, (void *)&colaPacientes[i]); //Se crea el hilo para la solicitud
			
			i = nPacientes + 1;
			anyadido = true;
		}

		i++;
		
		if(i > nPacientes && anyadido == false){

			pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Solicitud ignorada.");
                        pthread_mutex_unlock(&mutexLog);
		}

		pthread_mutex_unlock(&mutexColaPacientes);

	}

}

void writeLogMessage(char *identifier, int id, char *msg)
{

        // Calculamos la hora actual
        time_t now = time(0);
        struct tm *tlocal = localtime(&now);
        char stnow[19];
        strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);
        // Escribimos en el log
        logFile = fopen("registroTiempos.log", "a");
        fprintf(logFile, "[%s] %s %d: %s\n", stnow, identifier, id, msg);
        fclose(logFile);
}

