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
	int Atendido; //0 - no, 1 - si esta siendo atendido, 2 - si ya ha sido atendido, 3 - no ha tenido reaccion, 4 - ha tenido reaccion, 5- ha sido atendido con reaccion, 6- esta decidiendo si se queda
	int Tipo; //Edad de los pacientes, 0-15-junior, 16-59-medio, 60-100 senior, 0 - junior, 1 - medio, 2 - senior
	int Serologia; // 1-Participan, 0-No, 2-Ya participó
	int Posicion; //Guarda la posicion en la que se encuentra en la cola
	pthread_t paciente;

} Pacientes;

typedef struct{

	int ID;
	int Tipo; //0-junior, 1-medio, 2-senior
	int nPacientes; //Contador pacientes
	pthread_t enfermero; 

}Enfermero;

//Variables

int contadorPacientes, terminado, nPacientes;
pthread_mutex_t mutexLog, mutexColaPacientes, mutexEstadistico, mutexAleatorios, mutexColaEnfermero;
pthread_cond_t condReaccion, condSerologia, condMarchar;
bool cond = false;

Pacientes colaPacientes[15];
Enfermero colaEnfermero[3];

//hilos
pthread_t medico, estadistico;

FILE *logFile;

void inicializarEnfermeros(int nEnfermeros);
void nuevoPaciente(int signal);
void *accionesPaciente(void *arg);
void *accionesEnfermero(void *arg);
void *accionesMedico(void *arg);
void *accionesEstadistico(void *arg);
//void finalizar(int signal);
void writeLogMessage(char *identifier, int id, char *msg);
int calculaAleatorios(int min, int max);
int calculaColaMasGrande();


int main(int argc, char *argv[]){

	printf("main\n");

	struct sigaction pacienteJunior, pacienteMedio, pacienteSenior;//, terminar

	pacienteJunior.sa_handler = nuevoPaciente;
	pacienteMedio.sa_handler = nuevoPaciente;
	pacienteSenior.sa_handler = nuevoPaciente;
	
	//terminar.sa_handler = finalizar;

	sigaction(SIGUSR1, &pacienteJunior, NULL);	//Manejo SIGUSR1 para los pacientes junior
        sigaction(SIGUSR2, &pacienteMedio, NULL);	//Manejo SIGUSR2 para los pacientes Medios
	sigaction(SIGPIPE, &pacienteSenior, NULL);	//Manejo SIGPIPE para los pacientes Senior	
        //sigaction(SIGINT, &terminar, NULL);		//Manejo SIGINT para terminar

	//Inicializar Recursos
	pthread_mutex_init(&mutexLog, NULL);
	pthread_mutex_init(&mutexColaPacientes, NULL);
	pthread_mutex_init(&mutexEstadistico, NULL);
	pthread_mutex_init(&mutexAleatorios, NULL);
	pthread_mutex_init(&mutexColaEnfermero, NULL);
	pthread_cond_init(&condReaccion, NULL);
	pthread_cond_init(&condSerologia, NULL);
	pthread_cond_init(&condMarchar, NULL);
	
	srand(time(NULL));
	contadorPacientes = 0;
	terminado = 0;
	nPacientes = 0;

	//Comprobamos los argumentos

	if(argc==2) {
		nPacientes = atoi(argv[1]);
    }

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
	inicializarEnfermeros(3);
	pthread_create(&medico, NULL, &accionesMedico, NULL);
	pthread_create(&estadistico, NULL, &accionesEstadistico, NULL);

	while(terminado == 0){
        pause();
    }
    pthread_exit(NULL);
}

void inicializarEnfermeros(int nEnfermeros){

	int i = 0;

	for(i = 0; i<nEnfermeros; i++){

		pthread_mutex_lock(&mutexColaEnfermero);

		printf("He muteado la cola en inicializar_ENFERMERO\n");

		if(i == 0){

			colaEnfermero[i].ID = i;
			colaEnfermero[i].Tipo = 0;
	
		}else if(i == 1){

			colaEnfermero[i].ID = i;
			colaEnfermero[i].Tipo = 1;

		}else if(i == 2){

			colaEnfermero[i].ID = i;
			colaEnfermero[i].Tipo = 2;

		}

		colaEnfermero[i].nPacientes = 0;

		pthread_create(&colaEnfermero[i].enfermero, NULL, accionesEnfermero, (void *)&colaEnfermero[i]); //Se crea el hilo para ese enfermero
		pthread_mutex_unlock(&mutexColaEnfermero);
	}

}

void nuevoPaciente(int signal){

	printf("nuevoP\n");

	int i = 0, j;
	bool anyadido = false;

	pthread_mutex_lock(&mutexColaPacientes);

	printf("He muteado la cola en nuevo_PACIENTE\n");

	while(i<=nPacientes && anyadido == false){
		
		if(colaPacientes[i].ID == 0){
		
			contadorPacientes += 1;
	
			colaPacientes[i].ID = contadorPacientes;
			colaPacientes[i].Atendido = 0;
			colaPacientes[i].Posicion = i;
			
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

	printf("accionesP\n");

	Pacientes paciente = *((Pacientes *)arg);

	int aleatorio, aleatorio2, aleatorio3, aleatorio4, i, posici;   //Calculamos el aleatorio para saber si se cansa de esperar
	 //Calculamos el aleatorio para saber si se lo piensa mejor
	 //Calculamos el aleatorio del 70 por ciento que queda para saber si se va al baño EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
	//Calculamos si el Paciente tiene reaccion

	pthread_mutex_lock(&mutexLog);

	printf("He entrado en el mutex de accionesP\n");

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

	printf("TOMATE\n");

	pthread_mutex_lock(&mutexColaPacientes);

	printf("PACIENTE atendido: %d, ID: %d\n", paciente.Atendido, paciente.ID);

	while(paciente.Atendido == 0 && paciente.ID != 0){
		
		

		aleatorio = calculaAleatorios(0, 100);
		aleatorio2 = calculaAleatorios(0, 100);
		aleatorio3 = calculaAleatorios(0, 100);

		printf("PRUEBA\n");
		if(aleatorio <=20 || aleatorio2 <= 10){ //Se van por cansancio o se lo piensa mejor

			pthread_mutex_lock(&mutexLog);
            writeLogMessage("Paciente", paciente.ID, "Se va de la cola, por espera o por cansancio.");
            pthread_mutex_unlock(&mutexLog);

			//pthread_mutex_lock(&mutexColaPacientes);

			printf("He muteado la cola en PACIENTES\n");

			//Se libea el espacio de la cola, porque si el ID esta a 0 es como si no estuviera		

			posici = paciente.Posicion;

            for (i = posici; i < nPacientes; i++)
            {
                colaPacientes[i].Posicion = colaPacientes[i].Posicion-1;
                colaPacientes[i] = colaPacientes[i + 1];
            }
            colaPacientes[nPacientes - 1].ID = 0;	
	
			pthread_mutex_unlock(&mutexColaPacientes);
			printf("PRUEBA 2\n");
			pthread_exit(NULL);

		}else{
			pthread_mutex_unlock(&mutexColaPacientes);
			if(aleatorio3 <= 5){ //Va al baño y pierde el turno

				pthread_mutex_lock(&mutexLog);
            	writeLogMessage("Paciente", paciente.ID, "Va al baño, pierde el turno.");
            	pthread_mutex_unlock(&mutexLog);

				pthread_mutex_lock(&mutexColaPacientes);

				printf("He muteado la cola en PACIENTES 3\n");

				//Se libea el espacio de la cola, porque si el ID esta a 0 es como si no estuviera	
			for (i = posici; i < nPacientes; i++)
            {
                colaPacientes[i].Posicion = colaPacientes[i].Posicion-1;
                colaPacientes[i] = colaPacientes[i + 1];
            }
            colaPacientes[nPacientes - 1].ID = 0;		
	
			pthread_mutex_unlock(&mutexColaPacientes);

			pthread_exit(NULL);

			}else{

				pthread_mutex_lock(&mutexLog);
            	writeLogMessage("Paciente", paciente.ID, "Se queda");
            	pthread_mutex_unlock(&mutexLog);
			}

			sleep(3);

		}
		
		while(1){ //Esperamos a que haya sido atendido
			pthread_mutex_lock(&mutexColaPacientes);
			if(colaPacientes[i].Atendido != 2){
				pthread_mutex_unlock(&mutexColaPacientes);
				sleep(1);
			}
		}

		
		
		aleatorio4 = calculaAleatorios(0, 100);

		if(aleatorio4 <= 10){ //Miramos a ver si le da reaccion

			pthread_mutex_lock(&mutexLog);
			writeLogMessage("Paciente", paciente.ID, "El paciente ha tenido reaccion.");
            pthread_mutex_unlock(&mutexLog);

			pthread_mutex_lock(&mutexColaPacientes);

			printf("He muteado la cola en PACIENTES 4\n");

			colaPacientes[i].Atendido = 4;
			printf("ID: %d, atendido: %d\n", paciente.ID, paciente.Atendido);
			pthread_mutex_unlock(&mutexColaPacientes);

			while(paciente.Atendido !=5){ //espera a que haya sido atendido por el medico
				sleep(1);		
			}
			printf("Conde te queremos\n");

		}else{ //Si no le da reaccion

			pthread_mutex_lock(&mutexLog);
			writeLogMessage("Paciente", paciente.ID, "El paciente no ha tenido reaccion.");
            pthread_mutex_unlock(&mutexLog);
			
			pthread_mutex_lock(&mutexColaPacientes);////////////////////////////////////////////////////////////////////////////////7

			printf("He muteado la cola en PACIENTES 5\n");

			colaPacientes[i].Atendido = 3;
			//pthread_mutex_unlock(&mutexColaPacientes);

			if(calculaAleatorios(0, 100) <= 100){ //Calculamos si decide participar en el estudio

				//pthread_mutex_lock(&mutexColaPacientes);

				printf("He muteado la cola en PACIENTES 6\n");

				colaPacientes[i].Serologia = 1;
				pthread_mutex_unlock(&mutexColaPacientes);

				cond = true;
				pthread_cond_signal(&condSerologia);////////////////////////////////////////////////////////////////////////////////////////////////////////////

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "Estoy listo para el estudio.");
               	pthread_mutex_unlock(&mutexLog);

				pthread_cond_wait(&condMarchar,&mutexColaPacientes);
				pthread_mutex_unlock(&mutexEstadistico);

				pthread_mutex_unlock(&mutexColaPacientes);

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "Me marcho del estudio.");
               	pthread_mutex_unlock(&mutexLog);

				

			}else{

				//pthread_mutex_lock(&mutexColaPacientes);

				printf("He muteado la cola en PACIENTES 7\n");

				
				//pthread_mutex_unlock(&mutexColaPacientes);

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "No participo en el estudio asique me voy.");
            	pthread_mutex_unlock(&mutexLog);
                
                for (i = posici; i < nPacientes; i++)
          		{
                    colaPacientes[i].Posicion = colaPacientes[i].Posicion-1;
                    colaPacientes[i] = colaPacientes[i + 1]; 
            	}
           		colaPacientes[nPacientes - 1].ID = 0;

				pthread_mutex_unlock(&mutexColaPacientes);

			}
		
			//pthread_mutex_unlock(&mutexColaPacientes);
		}

		//pthread_mutex_lock(&mutexColaPacientes);

		printf("He muteado la cola en PACIENTES 8\n");

		pthread_mutex_lock(&mutexColaPacientes);

		printf("Estoy en el mutex de pacientes 8\n");

		if(colaPacientes[paciente.Posicion+1].ID!=0){

			colaPacientes[i].ID = 0;

		}else{

			for(i = paciente.Posicion; i<nPacientes; i++){
				if(colaPacientes[i+1].ID!=0){
					paciente.ID = 0;
					colaPacientes[i] = colaPacientes[i+1];
				}else{
		
					colaPacientes[i].ID = 0;
				}
			}
		}
		pthread_mutex_unlock(&mutexColaPacientes);

	}

	
	pthread_exit(NULL);

}

void *accionesEnfermero(void *arg){

	printf("accionesE\n");

	Enfermero enfermero = *((Enfermero *)arg);

	//Declaracion de variables
	bool mismoTipo = false;
	bool distintoTipo = false;
	bool encontrado = false;
	bool pacienteAtendido = false;

	int i, tiempoEspera, tipoAtencion;

	while(1){

		if(enfermero.ID == 0){//preguntamos en que enfermero estamos
			//if(colaPacientes[0].ID != 0){
			
			pthread_mutex_lock(&mutexColaPacientes);

			if(colaPacientes[0].ID != 0){

				//printf("ESTAMO ACTIVO BBY enfermero 0 mismo\n");

				i = 0;
				encontrado = false;
				pacienteAtendido = false;
				while(i < nPacientes && colaPacientes[i].ID != 0 && pacienteAtendido == false && colaPacientes[i].Atendido == 0){/////////////////////////CAMBIAR EN TODOS LADOS ULTIMA CONDICIION
					//El paciente es el mismo tipo que su enfermero	


					if( colaPacientes[i].Tipo == enfermero.Tipo){

						encontrado = true;
						mismoTipo = true;
						pacienteAtendido = true;

						colaPacientes[i].Atendido = 1; //Esta siendo atentido

						pthread_mutex_unlock(&mutexColaPacientes);

						tipoAtencion = calculaAleatorios(1, 100);

						//Tiene todo en regla
						if(tipoAtencion <= 80)
						{
							tiempoEspera = calculaAleatorios(1, 4);
						//Está mal identificado
						}else if(tipoAtencion > 80 && tipoAtencion <= 90)
						{
							tiempoEspera = calculaAleatorios(2, 6);
						//Tiene catarro o gripe
						}else
						{
							tiempoEspera = calculaAleatorios(6, 10);
						}

						pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Enfermero", enfermero.ID, "Comienza la actividad.");
                        pthread_mutex_unlock(&mutexLog);

		                sleep(tiempoEspera);

		                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad.");
                        pthread_mutex_unlock(&mutexLog);

		                pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad por...");//Varios tipos*****************************************
                        pthread_mutex_unlock(&mutexLog);

                        pthread_mutex_lock(&mutexColaPacientes);
		                colaPacientes[i].Atendido = 2; //Ya ha sido atentido
		                pthread_mutex_unlock(&mutexColaPacientes);

		                enfermero.nPacientes++;
		                
		            }
		            i++;
				}//Fin while(mismo tipo paciente)
				
				if(encontrado == false){//El paciente es de distinto tipo que el enfermero
					i = 0;
					
					pacienteAtendido = false;

					//printf("ESTAMO ACTIVO BBY enfermero 0 diferente\n");

					while(i < nPacientes && colaPacientes[i].ID != 0 && pacienteAtendido == false){
						if(colaPacientes[i].Atendido == 0){
							distintoTipo = true;
							pacienteAtendido = true;

							printf("He muteado la cola en ENFERMERO\n");

							colaPacientes[i].Atendido = 1; //Esta siendo atentido

							pthread_mutex_unlock(&mutexColaPacientes);

							tipoAtencion = calculaAleatorios(1, 100);

							//Tiene todo en regla
							if(tipoAtencion <= 80)
							{
								tiempoEspera = calculaAleatorios(1, 4);
							//Está mal identificado
							}else if(tipoAtencion > 80 && tipoAtencion <= 90)
							{
								tiempoEspera = calculaAleatorios(2, 6);
							//Tiene catarro o gripe
							}else
							{
								tiempoEspera = calculaAleatorios(6, 10);
							}

							pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Comienza la actividad.");
	                        pthread_mutex_unlock(&mutexLog);
			                
			                sleep(tiempoEspera);

			                pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad.");
	                        pthread_mutex_unlock(&mutexLog);

			                pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad por...");//Varios tipos*****************************************
	                        pthread_mutex_unlock(&mutexLog);

	                        pthread_mutex_lock(&mutexColaPacientes);
			                colaPacientes[i].Atendido = 2; //Ya ha sido atentido
			                pthread_mutex_unlock(&mutexColaPacientes);

			                //pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

			                enfermero.nPacientes++;
			            }

		                i++;
					}//fin while(distinto tipo de paciente)

				}//fin if(no se ha encontrado paciente del mismo tipo)

				//Miramos a ver si se tiene que ir a tomar café
	            if(enfermero.nPacientes == 5){
	                sleep(5);
	            }

	            //Si no tiene pacientes a los que atender volverá al principio
	            if(mismoTipo == false && distintoTipo == false){
	            	sleep(1);
	            	i = 0;
	            }
			}else{
				pthread_mutex_unlock(&mutexColaPacientes);
			}
			//fin enfermero 0
		}else if(enfermero.ID == 1){

			//if(colaPacientes[0].ID != 0){
			pthread_mutex_lock(&mutexColaPacientes);
			pacienteAtendido = false;

			if(colaPacientes[0].ID != 0){

				//printf("ESTAMO ACTIVO BBY enfermero 1 mismo\n");

				i = 0;
				encontrado = false;

				while(i < nPacientes && colaPacientes[i].ID != 0 && pacienteAtendido == false){

					//El paciente es el mismo tipo que su enfermero							
					if(colaPacientes[i].Atendido == 0 && colaPacientes[i].Tipo == enfermero.Tipo){

						encontrado = true;
						mismoTipo = true;
						pacienteAtendido = true;

						

						colaPacientes[i].Atendido = 1; //Esta siendo atentido

						pthread_mutex_unlock(&mutexColaPacientes);

						tipoAtencion = calculaAleatorios(1, 100);

						//Tiene todo en regla
						if(tipoAtencion <= 80)
						{
							tiempoEspera = calculaAleatorios(1, 4);
						//Está mal identificado
						}else if(tipoAtencion > 80 && tipoAtencion <= 90)
						{
							tiempoEspera = calculaAleatorios(2, 6);
						//Tiene catarro o gripe
						}else
						{
							tiempoEspera = calculaAleatorios(6, 10);
						}

						pthread_mutex_lock(&mutexLog);
	                    writeLogMessage("Enfermero", enfermero.ID, "Comienza la actividad.");
	                    pthread_mutex_unlock(&mutexLog);
		                
		                sleep(tiempoEspera);

		                pthread_mutex_lock(&mutexLog);
	                    writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad.");
	                    pthread_mutex_unlock(&mutexLog);

		                pthread_mutex_lock(&mutexLog);
	                    writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad por...");//Varios tipos*****************************************
	                    pthread_mutex_unlock(&mutexLog);

	                    pthread_mutex_lock(&mutexColaPacientes);
		                colaPacientes[i].Atendido = 2; //Ya ha sido atentido
		             	pthread_mutex_unlock(&mutexColaPacientes);

		                enfermero.nPacientes++;
		                
		            }
		            i++;
				}//Fin while(mismo tipo paciente)
					
				if(encontrado == false){//El paciente es de distinto tipo que el enfermero
					i = 0;
					
					pacienteAtendido = false;

					//printf("ESTAMO ACTIVO BBY enfermero 1 diferente\n");

					while(i < nPacientes && colaPacientes[i].ID != 0 && pacienteAtendido == false){
						if(colaPacientes[i].Atendido == 0){
							distintoTipo = true;
							pacienteAtendido = true;

							printf("He muteado la cola en ENFERMERO\n");

							colaPacientes[i].Atendido = 1; //Esta siendo atentido

							pthread_mutex_unlock(&mutexColaPacientes);

							tipoAtencion = calculaAleatorios(1, 100);

							//Tiene todo en regla
							if(tipoAtencion <= 80)
							{
								tiempoEspera = calculaAleatorios(1, 4);
							//Está mal identificado
							}else if(tipoAtencion > 80 && tipoAtencion <= 90)
							{
								tiempoEspera = calculaAleatorios(2, 6);
							//Tiene catarro o gripe
							}else
							{
								tiempoEspera = calculaAleatorios(6, 10);
							}

							pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Comienza la actividad.");
	                        pthread_mutex_unlock(&mutexLog);
			                
			                sleep(tiempoEspera);

			                pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad.");
	                        pthread_mutex_unlock(&mutexLog);

			                pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad por...");//Varios tipos*****************************************
	                        pthread_mutex_unlock(&mutexLog);

	                        pthread_mutex_lock(&mutexColaPacientes);
			                colaPacientes[i].Atendido = 2; //Ya ha sido atentido
			                printf("He cambiado el atendido a 2\n");
			                pthread_mutex_unlock(&mutexColaPacientes);

			                enfermero.nPacientes++;
			            }

		                i++;
					}//fin while(distinto tipo de paciente)

				}//fin if(no se ha encontrado paciente del mismo tipo)

					//Miramos a ver si se tiene que ir a tomar café
		            if(enfermero.nPacientes == 5){
		                sleep(5);
		            }

		            //Si no tiene pacientes a los que atender volverá al principio
		            if(mismoTipo == false && distintoTipo == false){
		            	sleep(1);
		            	i = 0;
		            }
			
		            //fin enfermero 1
	        }else{
	        	pthread_mutex_unlock(&mutexColaPacientes);
	        }
		}else if(enfermero.ID == 2){

			pthread_mutex_lock(&mutexColaPacientes);

			if(colaPacientes[0].ID != 0){

				pacienteAtendido = false;

				//printf("ESTAMO ACTIVO BBY enfermero 2 mismo\n");
				
				i = 0;
				encontrado = false;

				while(i < nPacientes && colaPacientes[i].ID != 0 && pacienteAtendido == false){

					//El paciente es el mismo tipo que su enfermero							
					if(colaPacientes[i].Atendido == 0 && colaPacientes[i].Tipo == enfermero.Tipo){

						encontrado = true;
						mismoTipo = true;
						pacienteAtendido = true;

						colaPacientes[i].Atendido = 1; //Esta siendo atentido

						pthread_mutex_unlock(&mutexColaPacientes);

						tipoAtencion = calculaAleatorios(1, 100);

						//Tiene todo en regla
						if(tipoAtencion <= 80)
						{
							tiempoEspera = calculaAleatorios(1, 4);
						//Está mal identificado
						}else if(tipoAtencion > 80 && tipoAtencion <= 90)
						{
							tiempoEspera = calculaAleatorios(2, 6);
						//Tiene catarro o gripe
						}else
						{
							tiempoEspera = calculaAleatorios(6, 10);
						}

						pthread_mutex_lock(&mutexLog);
	                    writeLogMessage("Enfermero", enfermero.ID, "Comienza la actividad.");
	                    pthread_mutex_unlock(&mutexLog);
		                
		                sleep(tiempoEspera);

		                pthread_mutex_lock(&mutexLog);
	                    writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad.");
	                    pthread_mutex_unlock(&mutexLog);

		                pthread_mutex_lock(&mutexLog);
	                    writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad por...");//Varios tipos*****************************************
	                    pthread_mutex_unlock(&mutexLog);

	                    pthread_mutex_lock(&mutexColaPacientes);
		                colaPacientes[i].Atendido = 2; //Ya ha sido atentido
		                pthread_mutex_unlock(&mutexColaPacientes);
		             
		                enfermero.nPacientes++;
		                
		            }
		            i++;
				}//Fin while(mismo tipo paciente)
				
				if(encontrado == false){//El paciente es de distinto tipo que el enfermero
					i = 0;
					
					pacienteAtendido = false;
					
					//printf("ESTAMO ACTIVO BBY enfermero 2 diferente\n");

					while(i < nPacientes && colaPacientes[i].ID != 0 && pacienteAtendido == false){
						if(colaPacientes[i].Atendido == 0){
							distintoTipo = true;
							pacienteAtendido = true;

							printf("He muteado la cola en ENFERMERO\n");

							colaPacientes[i].Atendido = 1; //Esta siendo atentido

							pthread_mutex_unlock(&mutexColaPacientes);

							tipoAtencion = calculaAleatorios(1, 100);

							//Tiene todo en regla
							if(tipoAtencion <= 80)
							{
								tiempoEspera = calculaAleatorios(1, 4);
							//Está mal identificado
							}else if(tipoAtencion > 80 && tipoAtencion <= 90)
							{
								tiempoEspera = calculaAleatorios(2, 6);
							//Tiene catarro o gripe
							}else
							{
								tiempoEspera = calculaAleatorios(6, 10);
							}

							pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Comienza la actividad.");
	                        pthread_mutex_unlock(&mutexLog);
			                
			                sleep(tiempoEspera);

			                pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad.");
	                        pthread_mutex_unlock(&mutexLog);

			                pthread_mutex_lock(&mutexLog);
	                        writeLogMessage("Enfermero", enfermero.ID, "Finaliza la actividad por...");//Varios tipos*****************************************
	                        pthread_mutex_unlock(&mutexLog);

	                        pthread_mutex_lock(&mutexColaPacientes);
			                colaPacientes[i].Atendido = 2; //Ya ha sido atentido
			                pthread_mutex_unlock(&mutexColaPacientes);

			               // pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

			                enfermero.nPacientes++;
			            }

		                i++;
					}//fin while(distinto tipo de paciente)
				}//fin if(no se ha encontrado paciente del mismo tipo)

				//Miramos a ver si se tiene que ir a tomar café
	            if(enfermero.nPacientes == 5){
	                sleep(5);
	            }

	            //Si no tiene pacientes a los que atender volverá al principio
	            if(mismoTipo == false && distintoTipo == false){
	            	sleep(1);
	            	i = 0;
	            }
        	}else{
        		pthread_mutex_unlock(&mutexColaPacientes);
        	}
		}//fin enfermero 2

	}//Fin while 1

}//FIN METODO


void *accionesMedico(void *arg){

	printf("accionesM\n");

	int i;
	int tipoAtencion, tiempoEspera, reaccion, masPacientes;
	bool encontrado;
	bool pacienteConReaccion;
	
	while(1)
	{
		
		pthread_mutex_lock(&mutexColaPacientes);
		if(colaPacientes[0].ID != 0)
		{
			//printf("accionesM_2\n");
			//Busco el primer paciente en la cola que ha dado reacción
			encontrado = false;
			i = 0;
			while(i<nPacientes && encontrado == false)
			{
				if(colaPacientes[i].ID != 0 && colaPacientes[i].Atendido == 4)
				{
					printf("He muteado la cola en MEDICO\n");
					encontrado = true;
				}else
				{
					i++;
				}
			}

			//Si no ha encontrado ningún paciente que haya dado reacción busco pacientes en la cola con más solicitudes
			if(encontrado == false)
			{
				/*
				pthread_mutex_lock(&mutexLog);
	            writeLogMessage("Medico", 1, "Como no había pacientes con reacción ayudo a los enfermeros");
				pthread_mutex_unlock(&mutexLog);
				*/

				masPacientes = calculaColaMasGrande();
				i = 0;
				//Busco el primer paciente del tipo seleccionado
				while(i<nPacientes && encontrado == false)
				{
					if(colaPacientes[i].ID != 0 && colaPacientes[i].Tipo == masPacientes && colaPacientes[i].Atendido == 0)
					{
						encontrado = true;
					}else
					{
						i++;
					}
				}
			
			}else{
				pthread_mutex_lock(&mutexLog);
	            writeLogMessage("Medico", 1, "Hay un paciente con reacción al que atender");
				pthread_mutex_unlock(&mutexLog);
			}
			
			//Si ha encontrado un paciente al que atender comienza el proceso de vacunación

			pacienteConReaccion = false;

			if(encontrado == true){

				if(colaPacientes[i].Atendido == 4)
				{
					colaPacientes[i].Atendido = 5;
					pacienteConReaccion = true;
				}else
				{
					colaPacientes[i].Atendido = 2;
				}

				pthread_mutex_unlock(&mutexColaPacientes);

				tipoAtencion = calculaAleatorios(1, 100);

				//Tiene todo en regla
				if(tipoAtencion <= 80)
				{
					tiempoEspera = calculaAleatorios(1, 4);
				//Está mal identificado
				}else if(tipoAtencion > 80 && tipoAtencion <= 90)
				{
					tiempoEspera = calculaAleatorios(2, 6);
				//Tiene catarro o gripe
				}else
				{
					tiempoEspera = calculaAleatorios(6, 10);
				}
				
				if(pacienteConReaccion == true){

					tiempoEspera += 5;
				}

				pthread_mutex_lock(&mutexLog);
	            writeLogMessage("Medico", 1, "Comienzo a atender al paciente");
				pthread_mutex_unlock(&mutexLog);

				sleep(tiempoEspera);

				pthread_mutex_lock(&mutexLog);
	            writeLogMessage("Medico", 1, "He acabado de atender al paciente correctamente");
				pthread_mutex_unlock(&mutexLog);

				printf("El tiempo de espera es: %d\n", tiempoEspera);

				if(pacienteConReaccion == true)
				{
					pthread_mutex_lock(&mutexLog);
		            writeLogMessage("Medico", 1, "Como ya ha sido atendido el paciente se marcha del consultorio");
					pthread_mutex_unlock(&mutexLog);
				}

			//Si no  ha encontrado ningún paciente espera 1 segundo antes de buscar otra vez
			}else
			{
				pthread_mutex_unlock(&mutexColaPacientes);
				sleep(1);
			}
		}else{//Fin del if
			
			pthread_mutex_unlock(&mutexColaPacientes);
			sleep(1);

		}
	}//Fin del while
}//Fin del metodo



void *accionesEstadistico(void *arg){

	printf("accionesEst\n");

	int i;
	pthread_mutex_lock(&mutexEstadistico);

	bool pacienteEstudio = false;
	i = 0;
	
	
	while(1)
	{
		while(cond == false){
		printf("accionesEst2\n");
		pthread_cond_wait(&condSerologia,&mutexEstadistico);
		printf("HOLIIII\n");
		}
		pthread_mutex_lock(&mutexColaPacientes);
		while(i < nPacientes && !pacienteEstudio)
		{
			if(colaPacientes[i].Serologia == 1){
				pthread_mutex_unlock(&mutexColaPacientes);
				pacienteEstudio = true;

				pthread_mutex_lock(&mutexLog);
                writeLogMessage("Estadistico", 1, "Comienza la actividad.");
                pthread_mutex_unlock(&mutexLog);

	            sleep(4);

	            pthread_mutex_lock(&mutexLog);
                writeLogMessage("Estadistico", 1, "Finaliza la actividad.");
                pthread_mutex_unlock(&mutexLog);

				pthread_cond_signal(&condMarchar);
			}
			i++;
		}
		
	}
	pthread_mutex_unlock(&mutexEstadistico);
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

int calculaColaMasGrande()
{
	int colaPacientesJunior = 0;
	int colaPacientesMedio = 0;
	int colaPacientesSenior = 0;
	int i;
	//Calculo cuantos pacientes de cada tipo hay esperando
	for(i = 0; i < nPacientes; i++)
	{
		if(colaPacientes[i].Tipo == 0)
		{
			colaPacientesJunior++;
		}else if(colaPacientes[i].Tipo == 1)
		{
			colaPacientesMedio++;
		}else if(colaPacientes[i].Tipo == 2)
		{
			colaPacientesSenior++;
		}
	}

	if(colaPacientesJunior > colaPacientesMedio && colaPacientesJunior > colaPacientesSenior)
	{
		return 0;
	}else if(colaPacientesMedio > colaPacientesJunior && colaPacientesMedio > colaPacientesSenior)
	{
		return 1;
	}else if(colaPacientesSenior > colaPacientesMedio && colaPacientesSenior > colaPacientesJunior)
	{
		return 2;
	}else
	{	
		return 0;
	}
}

/*void finalizar(int signal){
	int i;
	terminado = 1;
	pthread_mutex_lock(&mutexColaPacientes);
	for(i = 0; i<nPacientes; i++){
		colaPacientes[i].ID = 0;
		//terminar hilos
	}
	pthread_mutex_unlock(&mutexColaPacientes);
        exit(0);
}*/
