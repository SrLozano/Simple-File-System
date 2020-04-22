/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	Last revision 01/04/2020
 *
 */

#include "filesystem/filesystem.h" // Headers for the core functionality
#include "filesystem/auxiliary.h"  // Headers for auxiliary functions
#include "filesystem/metadata.h"   // Type and structure declaration of the file system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Inicializaciones de las estructuras a usar
TipoSuperbloque superbloque[1]; //Array de structs (typedef, hemos definido el tipo de Tiposuperbloque[0]) para distintos FS
TipoInodoDisco inodos[NUMINODO];
char i_map[NUMINODO] ; // Mapa de bits con los inodos (usado: i_map[x]=1 | libre: i_map[x]=0)    
char b_map[NUMBLOQUESDATO] ; // Mapa de bits con los bloques (usado: b_map[x]=1 | libre: b_map[x]=0)

struct //Información extra de apoyo

{
	//Nos indica por cada inodo si está abierto y en qué posición está en caso de que esté abierto.
	int posicion;
	int abierto; // 0: cerrado, 1: abierto

} inodosx[NUMINODO];

int esta_montado = 0 ; // Para saber si el disco está montado 0: falso, 1: verdadero

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{	
	// El sistema sera usado en discos de entre 460KiB y 600KiB, tal y como se especifa en los requisitos
	// Si queremos crear un disco fuera de los límites ERROR
	if (deviceSize < 471040 || deviceSize > 614400) return -1;

	//Inicializar valores por defecto
	superbloque[0].numMagico = 100383511; 
	superbloque[0].numInodos = NUMINODO;
	superbloque[0].numBloquesMapaInodos = 1; // Un bloque es suficiente para almacenar 48 inodos
    superbloque[0].numBloquesMapaDatos  = 1; // Un bloque es suficiente para almacenar los bloques correspondientes al tamanyo maximo de disco
    superbloque[0].primerInodo          = 3; // Número que indica el bloque del primer inodo
    superbloque[0].numBloquesDatos      = NUMBLOQUESDATO;
    superbloque[0].primerBloqueDatos    = 3 + NUMINODO; // El primer bloque de datos estará después de los inodos 

	// Se rellenan los mapas de inodos y bloques a 0 (Free)
	for (int i=0; i<superbloque[0].numInodos; i++) {
        i_map[i] = 0; // free
    }

    for (int i=0; i<superbloque[0].numBloquesDatos; i++) {
        b_map[i] = 0; // free
    }	

	// Se guardan los metadatos de deviceSize
	superbloque[0].tamDispositivo = deviceSize;

	/* Si finalmente hemos decidido meter INTEGRITY HAY QUE INICIARLIZARLO AQUÍ */
	
	// memset rellena con 0 tantas posiciones como sizeof(TipoInodoDisco) a partir de &(inodo[i]), es decir, rellena los inodos a 0
	for (int i=0; i<superbloque[0].numInodos; i++) {
        memset(&(inodos[i]), 0, sizeof(TipoInodoDisco) );  
    }

	printf("Hola\n");
	// Se desmonta el dispositivo y control de ERROR
	if (unmountFS() == -1) return -1;
	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */

int mountFS(void)
{	
	// Se comprueba si el sistema está ya montado
	if (1 == esta_montado) {
        return -1 ; //Error
    }

	// Los ficheros deben estar cerrados para poder montar
	for (int i=0; i<superbloque[0].numInodos; i++){
		if(inodosx[i].abierto == 1){
			return -1; //Error
		}
	}
	
	/* A continuación leemos los metadatos. Bread traera al FS los bloques del disco
	 que contenga los metadatos necesarios para montar el disco */
	
	printf("Hola Mount 1");

	// Leer del disco [bloque 0] al superbloque
	if (bread(DEVICE_IMAGE, 0, (char *)&superbloque[0]) == -1) return -1;
	
	printf("Hola Mount 2");

	// Comprobamos si el FS a montar es el que nosotros queremos
	if (superbloque[0].numMagico != 100383511) return -1;

	// Leer del disco [bloque 1] los bloques del mapa de inodos. Solo hay uno pero el for generaliza
	for (int i=0; i<superbloque[0].numBloquesMapaInodos; i++){
		if (bread(DEVICE_IMAGE, 1+i, (char *)i_map + i*BLOCK_SIZE) == -1) return -1;
	}

	// Leer del disco [bloque 2] los bloques del mapa de bloques de datos. Solo hay uno pero el for generaliza
	for (int i=0; i<superbloque[0].numBloquesMapaDatos; i++){
		if (bread(DEVICE_IMAGE, 1 + superbloque[0].numBloquesMapaInodos + i, (char *)b_map + i*BLOCK_SIZE) == -1) return -1;
	}

	// Leer los i-nodos del disco a memoria
    for (int i=0; i<(superbloque[0].numInodos*sizeof(TipoInodoDisco)/BLOCK_SIZE); i++) {
          if (bread(DEVICE_IMAGE, i + superbloque[0].primerInodo, ((char *)inodos + i*BLOCK_SIZE)) == -1) return -1;
    }

	/* ¿LOS BLOQUES NO SE LEEN A MEMORIAS? */

    esta_montado = 1 ; // 0: falso, 1: verdadero

	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{	
	// Se comprueba si el sistema NO está montado, pues no lo podría desmontar
	if (0 == esta_montado) {
        return -1 ; //Error
    }

	// Los ficheros deben estar cerrados para poder desmontar
	for (int i=0; i<superbloque[0].numInodos; i++){
		if(inodosx[i].abierto == 1){
			return -1; //Error
		}
	}
	
	printf("Hola 2\n");

	/* A continuación introducimos en el disco los metadatos [PERSISTENCIA]
		Brwrite copiara al disco los bloques que contengan los metadatos */ 
	
	// Escribir superbloque al disco [bloque 0]
	if (bwrite(DEVICE_IMAGE, 0, (char *)&superbloque[0]) == -1) return -1;

	// Escribir los bloques del mapa de inodos [bloque 1] Solo hay uno pero el for generaliza
	for (int i=0; i<superbloque[0].numBloquesMapaInodos; i++){
		if (bwrite(DEVICE_IMAGE, 1+i, (char *)i_map + i*BLOCK_SIZE) == -1) return -1;
	}

	// Escribir los bloques del mapa de bloques de datod [bloque 2] Solo hay uno pero el for generaliza
	for (int i=0; i<superbloque[0].numBloquesMapaDatos; i++){
		if (bwrite(DEVICE_IMAGE, 1 + superbloque[0].numBloquesMapaInodos + i, (char *)b_map + i*BLOCK_SIZE) == -1) return -1;
	}

	// Escribir los i-nodos a disco 
    for (int i=0; i<(superbloque[0].numInodos*sizeof(TipoInodoDisco)/BLOCK_SIZE); i++) {
          if (bwrite(DEVICE_IMAGE, i + superbloque[0].primerInodo, ((char *)inodos + i*BLOCK_SIZE)) == -1) return -1;
    }

	/* ¿LOS BLOQUES NO SE GUARDAN A DISCO? */

	esta_montado = 0 ; // 0: falso, 1: verdadero. Desmontar

	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	return -1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	return -1;
}

/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile (char * fileName)
{
    return -2;
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity (char * fileName)
{
    return -2;
}

/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{

    return -2;
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
    return -1;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
    return -1;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
    return -2;
}
