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
	int Atendido; //0 - no, 1 - si esta siendo atendido, 2 - si ya ha sido atendido, 3 - no ha tenido reaccion, 4 - ha tenido reaccion
	int Tipo; //Edad de los pacientes, 0-15-junior, 16-59-medio, 60-100 senior, 0 - junior, 1 - medio, 2 - senior
	int Serologia; // 1-Participan, 0-No
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

Pacientes colaPacientes[5];
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
	inicializarEnfermeros(3);
	pthread_create(&medico, NULL, &accionesMedico, NULL);
	pthread_create(&estadistico, NULL, &accionesEstadistico, NULL);

	 while(terminado == 0){
                pause;
        }
}

void inicializarEnfermeros(int nEnfermeros){

	int i = 0;

	for(i = 0; i<nEnfermeros; i++){

		pthread_mutex_lock(&mutexColaEnfermero);

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
printf("3\n");
	pthread_mutex_lock(&mutexColaPacientes);
	printf("%d\n",j);
printf("2\n");
	while(i<=nPacientes && anyadido == false){
		printf("1\n");
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

	int aleatorio = calculaAleatorios(0, 100); //Calculamos el aleatorio para saber si se cansa de esperar
	int aleatorio2 = calculaAleatorios(0, 100); //Calculamos el aleatorio para saber si se lo piensa mejor
	int aleatorio3 = calculaAleatorios(0, 70); //Calculamos el aleatorio del 70 por ciento que queda para saber si se va al baño EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
	int aleatorio4 = calculaAleatorios(0, 100); //Calculamos si el Paciente tiene reaccion
	int i;

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

	//pthread_mutex_lock(&mutexColaPacientes);

	while(paciente.Atendido == 0 && paciente.ID != 0){

		//pthread_mutex_unlock(&mutexColaPacientes);
		printf("eEEeeeeeee:%d\n",aleatorio);
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

			while(paciente.Atendido !=2){

				sleep(1);		
	
			}

		}else{ //Si no le da reaccion
			
			pthread_mutex_lock(&mutexColaPacientes);
			paciente.Atendido = 3;
			pthread_mutex_unlock(&mutexColaPacientes);

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



			}else{

				pthread_mutex_lock(&mutexColaPacientes);
				paciente.ID = 0;
				pthread_mutex_unlock(&mutexColaPacientes);

				pthread_mutex_lock(&mutexLog);
				writeLogMessage("Paciente", paciente.ID, "No participo en el estudio asique me voy.");
                        	pthread_mutex_unlock(&mutexLog);

			}
		}

		pthread_mutex_lock(&mutexColaPacientes);
	
		if(colaPacientes[paciente.Posicion+1].ID!=0){

			paciente.ID = 0;

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

	Enfermero enfermero = *((Enfermero *)arg);

	//Declaracion de variables
	bool mismoTipo = false;
	bool distintoTipo = false;
	int i, tiempoEspera, tipoAtencion;

	if(enfermero.ID == 0){

		pthread_mutex_lock(&mutexColaEnfermero);

		i = 0;
		while(i < nPacientes && colaPacientes[i].ID != 0){

			//El paciente es el mismo tipo que su enfermero
			if(colaPacientes[i].Tipo == enfermero.Tipo){

				mismoTipo = true;

				pthread_mutex_lock(&mutexColaPacientes);//bloqueamos al paciente

				colaPacientes[i].Atendido == 1; //Esta siendo atentido

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

                colaPacientes[i].Atendido = 2; //Ya ha sido atentido

                pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

                enfermero.nPacientes++;

                //El paciente es de distinto tipo que el enfermero
			}else{

				distintoTipo = true;

				pthread_mutex_lock(&mutexColaPacientes);//bloqueamos al paciente

				colaPacientes[i].Atendido == 1; //Esta siendo atentido

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

                colaPacientes[i].Atendido = 2; //Ya ha sido atentido

                pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

                enfermero.nPacientes++;
			}//fin llave else

			//Miramos a ver si se tiene que ir a tomar café
            if(enfermero.nPacientes == 5){
                sleep(5);
            }

            //Si no tiene pacientes a los que atender volverá al principio
            if(mismoTipo == false && distintoTipo == false){
            	sleep(1);
            	i = 0;
            }

            i++;
		}//fin bucle while

		pthread_mutex_unlock(&mutexColaEnfermero);

		//fin enfermero 0
	}else if(enfermero.ID == 1){

		pthread_mutex_lock(&mutexColaEnfermero);

		i = 0;
		while(i < nPacientes && colaPacientes[i].ID != 0){

			//El paciente es el mismo tipo que su enfermero
			if(colaPacientes[i].Tipo == enfermero.Tipo){

				mismoTipo = true;

				pthread_mutex_lock(&mutexColaPacientes);//bloqueamos al paciente

				colaPacientes[i].Atendido == 1; //Esta siendo atentido

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

                colaPacientes[i].Atendido = 2; //Ya ha sido atentido

                pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

                enfermero.nPacientes++;

                //El paciente es de distinto tipo que el enfermero
			}else{

				distintoTipo = true;

				pthread_mutex_lock(&mutexColaPacientes);//bloqueamos al paciente

				colaPacientes[i].Atendido == 1; //Esta siendo atentido

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

                colaPacientes[i].Atendido = 2; //Ya ha sido atentido

                pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

                enfermero.nPacientes++;
			}//fin llave else

			//Miramos a ver si se tiene que ir a tomar café
            if(enfermero.nPacientes == 5){
                sleep(5);
            }

            //Si no tiene pacientes a los que atender volverá al principio
            if(mismoTipo == false && distintoTipo == false){
            	sleep(1);
            	i = 0;
            }

            i++;
		}//fin bucle while

		pthread_mutex_unlock(&mutexColaEnfermero);
		
		//fin enfermero 1
	}else if(enfermero.ID == 2){

		pthread_mutex_lock(&mutexColaEnfermero);

		i = 0;
		while(i < nPacientes && colaPacientes[i].ID != 0){

			//El paciente es el mismo tipo que su enfermero
			if(colaPacientes[i].Tipo == enfermero.Tipo){

				mismoTipo = true;

				pthread_mutex_lock(&mutexColaPacientes);//bloqueamos al paciente

				colaPacientes[i].Atendido == 1; //Esta siendo atentido

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

                colaPacientes[i].Atendido = 2; //Ya ha sido atentido

                pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

                enfermero.nPacientes++;

                //El paciente es de distinto tipo que el enfermero
			}else{

				distintoTipo = true;

				pthread_mutex_lock(&mutexColaPacientes);//bloqueamos al paciente

				colaPacientes[i].Atendido == 1; //Esta siendo atentido

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

                colaPacientes[i].Atendido = 2; //Ya ha sido atentido

                pthread_mutex_unlock(&mutexColaPacientes);//desbloqueamos al paciente

                enfermero.nPacientes++;
			}//fin llave else

			//Miramos a ver si se tiene que ir a tomar café
            if(enfermero.nPacientes == 5){
                sleep(5);
            }

            //Si no tiene pacientes a los que atender volverá al principio
            if(mismoTipo == false && distintoTipo == false){
            	sleep(1);
            	i = 0;
            }

            i++;
		}//fin bucle while

		pthread_mutex_unlock(&mutexColaEnfermero);
		
	}//fin enfermero 2
		
}


void *accionesMedico(void *arg){
	int i;
	int tipoAtencion, tiempoEspera, reaccion, masPacientes;
	int colaPacientesJunior = 0;
	int colaPacientesMedio = 0;
	int colaPacientesSenior = 0;
	bool encontrado = false;
	bool pacienteConReaccion;

	printf("hola\n");

	while(1){
		pthread_mutex_lock(&mutexColaPacientes);
		//Busco el primer paciente en la cola que ha dado reacción
		encontrado = false;
		i = 0;
		while(i<nPacientes && encontrado == false)
		{
			if(colaPacientes[i].Atendido == 4)
			{
				encontrado = true;
			}else
			{
				i++;
			}
		}

		//Si no ha encontrado ningún paciente que haya dado reacción busco pacientes en la cola con más solicitudes
		if(encontrado == false)
		{
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
			

			//Busco que cola ha resultado más grande
			if(colaPacientesJunior > colaPacientesMedio && colaPacientesJunior > colaPacientesSenior)
			{
				masPacientes = 0;
			}else if(colaPacientesMedio > colaPacientesJunior && colaPacientesMedio > colaPacientesSenior)
			{
				masPacientes = 1;
			}else if(colaPacientesSenior > colaPacientesMedio && colaPacientesSenior > colaPacientesJunior)
			{
				masPacientes = 2;
			}

			//Busco el primer paciente del tipo seleccionado
			while(i<nPacientes && encontrado == false)
			{
				if(colaPacientes[i].Tipo == masPacientes)
				{
					encontrado = true;
				}else
				{
					i++;
				}
			}
			
		
		}
		
		//Si ha encontrado un paciente al que atender comienza el proceso de vacunación
		pacienteConReaccion = false;
		if(encontrado == true){
			if(colaPacientes[i].Atendido = 4)
			{
				pacienteConReaccion = true;
			}
			colaPacientes[i].Atendido = 2;
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

			pthread_mutex_lock(&mutexLog);
	            writeLogMessage("Medico", 1, "Como ya ha sido atendido el paciente se marcha del consultorio");
			pthread_mutex_unlock(&mutexLog);

		//Si no  ha encontrado ningún paciente espera 1 segundo antes de buscar otra vez
		}else
		{
			sleep(1);
		}
	}
}



void *accionesEstadistico(void *arg){

	printf("hola\n");

	int i;
	pthread_mutex_lock(&mutexEstadistico);

	bool pacienteEstudio = false;
	i = 0;
	
	while(i < nPacientes && !pacienteEstudio)
	{
		if(colaPacientes[i].Serologia == 1){
			pacienteEstudio = true;

			pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Estadistico", 1, "Comienza la actividad.");
                        pthread_mutex_unlock(&mutexLog);

            sleep(4);

            //USAR VARIABLE CONDICION mutexPacientesEstudio *******************************************************

            pthread_mutex_lock(&mutexLog);
                        writeLogMessage("Estadistico", 1, "Finaliza la actividad.");
                        pthread_mutex_unlock(&mutexLog);
		}
		i++;
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

