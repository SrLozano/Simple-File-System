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
struct TipoSuperbloque superbloque;
struct TipoInodoDisco inodo[NUMINODO];
char i_map[BLOCK_SIZE] ; // Mapa de bits con los inodos     
char b_map[BLOCK_SIZE] ; // Mapa de bits con los bloques 

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{	
	// El sistema sera usado en discos de entre 460KiB y 600KiB, tal y como se especifa en los requisitos
	// Si queremos crear un disco fuera de los límites ERROR
	
	//printf("%ld", deviceSize);

	if (deviceSize < 471040 || deviceSize > 614400) return -1;

	//Inicializar valores por defecto
	superbloque.numMagico = 100383511; 
	superbloque.numInodos = NUMINODO;
	superbloque.numBloquesMapaInodos = 1; // Un bloque es suficiente para almacenar 48 inodos
    superbloque.numBloquesMapaDatos  = 1; // Un bloque es suficiente para almacenar los bloques correspondientes al tamanyo maximo de disco
    superbloque.primerInodo          = 1;
    superbloque.numBloquesDatos      = NUMBLOQUESDATO;
    superbloque.primerBloqueDatos    = 48; // El primer bloque de datos estara despues de los inodos 

	// Se rellenan los mapas de inodos y bloques a 0 (Free)
	for (int i=0; i<superbloque.numInodos; i++) {
        i_map[i] = 0; // free
    }

    for (int i=0; i<superbloque.numBloquesDatos; i++) {
        b_map[i] = 0; // free
    }	

	// Se guardan los metadatos de deviceSize
	superbloque.tamDispositivo = deviceSize;

	/*Si finalmente hemos decidido meter INTEGRITY HAY QUE INICIARLIZARLO AQUÍ */
	
	// memset rellena con 0 tantas posiciones como sizeof(TipoInodoDisco) a partir de &(inodo[i]), es decir, rellena los inodos a 0
	for (int i=0; i<superbloque.numInodos; i++) {
        memset(&(inodo[i]), 0, sizeof(TipoInodoDisco) );  
    }
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
	/*¿DEBERÍAMOS METER UN FOR PARA GENERALIZAR?*/

	// Bread traera al FS los bloques del disco que contenga los metadatos necesarios para montar el disco 
	
	// Leer bloque 0 de disco y guardarlo en superbloque
	if (bread(DEVICE_IMAGE, 0, (char *)&superbloque) == -1) return -1;

	// Leer el bloque del mapa de inodos 
	if (bread(DEVICE_IMAGE, 1, (char *)i_map) == -1) return -1;
	
	// Lee el bloque para el mapa de bloques de datos
	if (bread(DEVICE_IMAGE, 2, (char *)b_map) == -1) return -1;

	// Leer los i-nodos de disco
    for (int i=0; i<(superbloque.numInodos*sizeof(TipoInodoDisco)/BLOCK_SIZE); i++) {
          if (bread(DEVICE_IMAGE, 3, ((char *)inodo + i*BLOCK_SIZE)) == -1) return -1;
    }

	// Se devuelve -1 en caso de error y 0 en caso de que la ejección sea correcta

	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	/*¿DEBERÍAMOS METER UN FOR PARA GENERALIZAR?*/

	// Brwrite copiara al disco los bloques que contengan los metadatos  
	
	// Escribir bloque 0 al disco 
	if (bwrite(DEVICE_IMAGE, 0, (char *)&superbloque) == -1) return -1;

	// Escribir el bloque del mapa de inodos 
	if (bwrite(DEVICE_IMAGE, 1, (char *)i_map) == -1) return -1;
	
	// Escribir el bloque para el mapa de bloques de datos
	if (bwrite(DEVICE_IMAGE, 2, (char *)b_map) == -1) return -1;

	// Escribir los i-nodos a disco 
    for (int i=0; i<(superbloque.numInodos*sizeof(TipoInodoDisco)/BLOCK_SIZE); i++) {
          if (bwrite(DEVICE_IMAGE, 3, ((char *)inodo + i*BLOCK_SIZE)) == -1) return -1;
    }

	// Se devuelve -1 en caso de error y 0 en caso de que la ejección sea correcta

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
