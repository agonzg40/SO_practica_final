#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>


//Estructuras
typedef struct{

	int ID;
	int Atendido; //0 - no, 1 - si esta siendo atendido, 2 - si ya ha sido atendido, 4 - ha tenido reaccion
	int Tipo; //Edad de los pacientes, 0-15-junior, 16-59-medio, 60-100 senior, 0 - junior, 1 - medio, 2 - senior
	int Serologia; // 1-Participan, 0-No
	pthread_t paciente;

} Pacientes;

typedef struct{

	int ID;
	int Tipo; //0-junior, 1-medio, 2-senior
	int Ocupado; //0-Ocupado, 1-Libre
	pthread_t enfermero; 

}Enfermero;

//Variables

int contadorPacientes, terminado, nPacientes;
pthread_mutex_t mutexLog, mutexColaPacientes, mutexPacientesEstudio, mutexAleatorios;
pthread_cond_t condReaccion, condSerologia, condMarchar;

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
int calculaAleatorios(int min, int max);


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
	pthread_mutex_init(&mutexAleatorios, NULL);
	pthread_cond_init(&condReaccion, NULL);
	pthread_cond_init(&condSerologia, NULL);
	pthread_cond_init(&condMarchar, NULL);
	
	srand(time(NULL));
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

	int i = 0;
	bool anyadido = false;

	pthread_mutex_lock(&mutexColaPacientes);

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
	}
	
	pthread_mutex_unlock(&mutexColaPacientes);

}

void *accionesPaciente(void *arg){

	Pacientes paciente = *((Pacientes *)arg);

	int aleatorio = calculaAleatorios(0, 100); //Calculamos el aleatorio para saber si se cansa de esperar
	int aleatorio2 = calculaAleatorios(0, 100); //Calculamos el aleatorio para saber si se lo piensa mejor
	int aleatorio3 = calculaAleatorios(0, 70); //Calculamos el aleatorio del 70 por ciento que queda para saber si se va al baño EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
	int aleatorio4 = calculaAleatorios(0, 100); //Calculamos si el Paciente tiene reaccion

	pthread_mutex_lock(&mutexLog);

	char mensaje[100] = "El paciente es de tipo: ";

	if(paciente.Tipo == 0){ //Comprobamos de que tipo es el paciente para el log
	
		strcat(mensaje, "junior");

	}else if(paciente.Tipo == 1){

		strcat(mensaje, "medio");
	
	}else{

		strcat(mensaje, "senior");
	
	}

        writeLogMessage("Paciente", paciente.ID, "Inicio del paciente");
	writeLogMessage("Paciente", paciente.ID, mensaje);

        pthread_mutex_unlock(&mutexLog);

	sleep(3);

	pthread_mutex_lock(&mutexColaPacientes);

	while(paciente.Atendido == 0 && paciente.ID != 0){

		pthread_mutex_unlock(&mutexColaPacientes);

		if(aleatorio<=20){ //Se van por cansancio

			pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Paciente", paciente.ID, "Se va de la cola, por espera.");
                        pthread_mutex_unlock(&mutexLog);

			pthread_mutex_lock(&mutexColaPacientes);

			paciente.ID = 0;//Se libea el espacio de la cola, porque si el ID esta a 0 es como si no estuviera			
	
			pthread_mutex_unlock(&mutexColaPacientes);
	
			pthread_exit(NULL);

		}else{

			if(aleatorio <= 10){ //Se lo piensa mejor
			
				pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Paciente", paciente.ID, "Se lo piensa mejor y se va.");
        	                pthread_mutex_unlock(&mutexLog);

				pthread_mutex_lock(&mutexColaPacientes);

				paciente.ID = 0;//Se libea el espacio de la cola, porque si el ID esta a 0 es como si no estuviera			
		
				pthread_mutex_unlock(&mutexColaPacientes);
		
				pthread_exit(NULL);	

			}else{

				if(aleatorio3 <= 5){ //Va al baño y pierde el turno

					pthread_mutex_lock(&mutexLog);
                        		writeLogMessage("Paciente", paciente.ID, "Se va de la cola, por espera.");
                        		pthread_mutex_unlock(&mutexLog);

					pthread_mutex_lock(&mutexColaPacientes);

					paciente.ID = 0;//Se libea el espacio de la cola, porque si el ID esta a 0 es como si no estuviera			
	
					pthread_mutex_unlock(&mutexColaPacientes);
	
					pthread_exit(NULL);

				}

			}
		}

		sleep(3);

		while(paciente.Atendido != 2){ //Esperamos a que haya sido atendido

			sleep(1);

		}

		if(aleatorio4 <= 10){ //Miramos a ver si le da reaccion

			pthread_mutex_lock(&mutexColaPacientes);
			paciente.Atendido = 4;
			pthread_mutex_unlock(&mutexColaPacientes);

			//pthread_cond_wait(&condReaccion, NULL);

		}else{ //Si no le da reaccion
			
			if(calculaAleatorios(0, 100) <= 25){ //Calculamos si decide participar en el estudio

				pthread_mutex_lock(&mutexColaPacientes);
				paciente.Serologia = 1;
				pthread_mutex_unlock(&mutexColaPacientes);

				pthread_cond_signal(&condSerologia);

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "Estoy listo para el estudio.");
                        	pthread_mutex_unlock(&mutexLog);

				//pthread_cond_wait(&condMarchar);

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "Me marcho del estudio.");
                        	pthread_mutex_unlock(&mutexLog);

				pthread_mutex_lock(&mutexColaPacientes);
				paciente.ID = 0;
				pthread_mutex_unlock(&mutexColaPacientes);

			}else{

				pthread_mutex_lock(&mutexColaPacientes);
				paciente.ID = 0;
				pthread_mutex_unlock(&mutexColaPacientes);

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "No participo en el estudio asique me voy.");
                        	pthread_mutex_unlock(&mutexLog);

			}
		}
	}
	
	pthread_exit(NULL);

	

}

void accionesEnfermero(int signal){

	//Variable para pacientes del mismo tipo que su enfermero
	bool pacienteMTipo = false;
	//Variable para pacientes de distinto tipo que su enfermero
	bool pacienteDTipo = false;

	if(Enfermero.ID == 0){
		int contador = 0;
		while(i < nPacientes && !pacienteMTipo)
		{
			if(colaPacientes[i].Tipo == Enfermero.Tipo){
				pacienteMTipo = true;

				colaPacientes[i].Atendido = 1;
				//CALCULAR EL TIEMPO DE REACCION, DIRECTAMENTE O EN UNA FUNCION*********************************************************************
				//FALTA TIPO DE ATENCION Y A PARTIR DE AHI CALCULOS

				pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Comienza la actividad(ENFERMERO).");
                        pthread_mutex_unlock(&mutexLog);

                //TIPO ATENCION*******************************************************************************************************************
                wait(tipoAtencion);

                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(ENFERMERO).");
                        pthread_mutex_unlock(&mutexLog);

                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(ENFERMERO) por...");//Varios tipos*****************************************
                        pthread_mutex_unlock(&mutexLog);

                colaPacientes.Atendido = 0;
                contador++;

			}else{
				pacienteDTipo = true;
				contador++;
			}

			if(!pacienteMTipo && !pacienteDTipo){
				wait(1);
			}

			if(contador == 5){
                	wait(5);
                }
		}
		i++;
	}

	if(Enfermero.ID == 1){
		int contador = 0;
		while(i < nPacientes && !pacienteMTipo)
		{
			if(colaPacientes[i].Tipo == Enfermero.Tipo){
				pacienteMTipo = true;

				colaPacientes[i].Atendido = 1;
				//CALCULAR EL TIEMPO DE REACCION, DIRECTAMENTE O EN UNA FUNCION*********************************************************************
				//FALTA TIPO DE ATENCION Y A PARTIR DE AHI CALCULOS

				pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Comienza la actividad(ENFERMERO).");
                        pthread_mutex_unlock(&mutexLog);

                //TIPO ATENCION*******************************************************************************************************************
                wait(tipoAtencion);

                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(ENFERMERO).");
                        pthread_mutex_unlock(&mutexLog);

                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(ENFERMERO) por...");//Varios tipos*****************************************
                        pthread_mutex_unlock(&mutexLog);

                colaPacientes.Atendido = 0;
                contador++;

			}else{
				pacienteDTipo = true;
				contador++;
			}

			if(!pacienteMTipo && !pacienteDTipo){
				wait(1);
			}

			if(contador == 5){
                	wait(5);
                }
		}
		i++;
	}

	if(Enfermero.ID == 2){
		int contador = 0;
		while(i < nPacientes && !pacienteMTipo)
		{
			if(colaPacientes[i].Tipo == Enfermero.Tipo){
				pacienteMTipo = true;

				colaPacientes[i].Atendido = 1;
				//CALCULAR EL TIEMPO DE REACCION, DIRECTAMENTE O EN UNA FUNCION*********************************************************************
				//FALTA TIPO DE ATENCION Y A PARTIR DE AHI CALCULOS

				pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Comienza la actividad(ENFERMERO).");
                        pthread_mutex_unlock(&mutexLog);

                //TIPO ATENCION*******************************************************************************************************************
                wait(tipoAtencion);

                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(ENFERMERO).");
                        pthread_mutex_unlock(&mutexLog);

                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(ENFERMERO) por...");//Varios tipos*****************************************
                        pthread_mutex_unlock(&mutexLog);

                colaPacientes.Atendido = 0;
                contador++;

			}else{
				pacienteDTipo = true;
				contador++;
			}

			if(!pacienteMTipo && !pacienteDTipo){
				wait(1);
			}

			if(contador == 5){
                	wait(5);
                }
		}
		i++;
	}
}

void accionesEstadistico(int signal){

	bool pacienteEstudio = false;

	pthread_mutex_lock(&estadistico);

	while(i < nPacientes && !pacienteEstudio)
	{
		if(colaPacientes[i].Serologia == 0){
			pacienteEstudio = true;

			pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Comienza la actividad(estadistico).");
                        pthread_mutex_unlock(&mutexLog);

            wait(4);

            //USAR VARIABLE CONDICION mutexPacientesEstudio *******************************************************

            pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Sistema", 1, "Finaliza la actividad(estadistico).");
                        pthread_mutex_unlock(&mutexLog);
		}
	}
	i++;

	pthread_mutex_unlock(&estadistico);
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

int calculaAleatorios(int min, int max)
{
        
        int aleatorio;
        pthread_mutex_lock(&mutexAleatorios);
        aleatorio = rand() % (max - min + 1) + min;
        pthread_mutex_unlock(&mutexAleatorios);
        return aleatorio;
}

