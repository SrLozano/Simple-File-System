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
TipoInodosBloque bloques_inodos[2];
char i_map[NUMINODO] ; 			// Mapa de bits con los inodos (usado: i_map[x]=1 | libre: i_map[x]=0)    
char b_map[NUMBLOQUESDATO] ; 	// Mapa de bits con los bloques (usado: b_map[x]=1 | libre: b_map[x]=0)


struct //Información extra de apoyo que no va a disco, se pierde al cargar en disco
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
	superbloque[0].numBloquesMapaInodos = 1; 			// Un bloque es suficiente para almacenar 48 inodos
    superbloque[0].numBloquesMapaDatos  = 1; 			// Un bloque es suficiente para almacenar los bloques correspondientes al tamanyo maximo de disco
    superbloque[0].primerInodo          = 3; 			// Número que indica el bloque del primer inodo
    superbloque[0].numBloquesDatos      = NUMBLOQUESDATO;
    superbloque[0].primerBloqueDatos    = 3 + 2;		// El primer bloque de datos estará después de los inodos, que están todos en 2 bloques 
	superbloque[0].tamDispositivo = deviceSize; 		// Se guardan los metadatos de deviceSize

	// Se rellenan los mapas de inodos y bloques a 0 (Free)
	for (int i=0; i<superbloque[0].numInodos; i++) {
        i_map[i] = 0; // free
    }

    for (int i=0; i<superbloque[0].numBloquesDatos; i++) {
        b_map[i] = 0; // free
    }	

	/* Si finalmente hemos decidido meter INTEGRITY HAY QUE INICIARLIZARLO AQUÍ */
	
	// memset rellena con 0 tantas posiciones como sizeof(TipoInodoDisco) a partir de &(bloques_inodos[i].inodos[j]), es decir, rellena los inodos a 0
	for (int i=0; i<BLOCKS_FOR_INODES; i++) {
		for(int j=0; j<NUMBER_INODES_PER_BLOCK; j++){
        	memset(&(bloques_inodos[i].inodos[j]), 0, sizeof(TipoInodoDisco) );  
		}
    }

	// Establecemos el valor de los bloques directos a -3 para saber cuales tenemos en uso y cuales no
	for (int i=0; i<BLOCKS_FOR_INODES; i++) {
		for(int j=0; j<NUMBER_INODES_PER_BLOCK; j++){
			for(int k=0; k<NUMBER_DIRECT_BLOCKS; k++)
			bloques_inodos[i].inodos[j].bloqueDirecto[k] = -3;
		}
	}

	// Se llama a la función sincronizar y control de ERROR
	if (my_sync() == -1){ 
		return -1;	
	}
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
	
	// Leer del disco [bloque 0] al superbloque
	if (bread(DEVICE_IMAGE, 0, (char *)&superbloque[0]) == -1) return -1;
	
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

	// Leer los i-nodos a disco 
    for (int i=0; i<BLOCKS_FOR_INODES; i++) {
          if (bread(DEVICE_IMAGE, i + superbloque[0].primerInodo, ((char *)bloques_inodos + i*BLOCK_SIZE)) == -1) return -1;
    }

    esta_montado = 1 ; // 0: falso, 1: verdadero

	return 0;
}

/*
 * @brief 	Synchronises disk and memory
 * @return 	0 if success, -1 otherwise.
 */

int my_sync(void)
{
	/* A continuación introducimos en el disco los metadatos [PERSISTENCIA]
	   Brwrite copiara al disco los bloques que contengan los metadatos */ 

	// Escribir superbloque al disco [bloque 0]
	if (bwrite(DEVICE_IMAGE, 0, (char *)&superbloque[0]) == -1) return -1;


	// Escribir los bloques del mapa de inodos [bloque 1] Solo hay uno pero el for generaliza
	for (int i=0; i<superbloque[0].numBloquesMapaInodos; i++){
		if (bwrite(DEVICE_IMAGE, 1+i, (char *)i_map + i*BLOCK_SIZE) == -1) return -1;
	}

	// Escribir los bloques del mapa de bloques de datos [bloque 2] Solo hay uno pero el for generaliza
	for (int i=0; i<superbloque[0].numBloquesMapaDatos; i++){
		if (bwrite(DEVICE_IMAGE, 1 + superbloque[0].numBloquesMapaInodos + i, (char *)b_map + i*BLOCK_SIZE) == -1) return -1;
	}

	// Escribir los i-nodos a disco 
    for (int i=0; i<BLOCKS_FOR_INODES; i++) {
          if (bwrite(DEVICE_IMAGE, i + superbloque[0].primerInodo, ((char *)bloques_inodos + i*BLOCK_SIZE)) == -1) return -1;
    }

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
	
	// Se llama a la función sincronizar y control de ERROR
	if (my_sync() == -1){ 
		return -1;	
	}

	esta_montado = 0 ; // 0: falso, 1: verdadero. Desmontar

	return 0;
}

/*
 * @brief 	computes the position of the inode map
 * @return 	array of position of the inode exists [block, position in the block], [-1, -1] otherwise.
 */
int * computePositionInodeMap(int positionMap)
{
	int bloque_buscar = -1;
	int inodo_buscar = -1;

	if (positionMap < NUMBER_INODES_PER_BLOCK){ // Es de los primeros 24 inodos asi que primer bloque de inodos
		bloque_buscar = 0;
		inodo_buscar = positionMap;
	} else if(positionMap < NUMINODO){ // Segundo bloque de inodos
		bloque_buscar = 1;
		inodo_buscar = positionMap - NUMBER_INODES_PER_BLOCK;
	}

	int *array = malloc (sizeof(int)*2);
	array[0] = bloque_buscar;
	array[1] = inodo_buscar;

	return array;
}

/*
 * @brief 	Allocates an inode
 * @return 	array of position of the inode if success, [-1, -1] otherwise.
 */

int* ialloc(void)
{
	int i;
	int *array = malloc (sizeof(int)*2);
	
	for(i=0; i<NUMINODO; i++){ // Recorremos el mapa de inodos
		
		if(i_map[i] == 0){  // Primer inodo libre, indicado por el mapa
            
			i_map[i] = 1;	// inodo ocupado ahora
			array = computePositionInodeMap(i); // Calculamos la posición dentro de nuestro sistema de ficheros
			
			if (array[0] == -1 || array[1]==-1)	{return array; } // Control de errores, no se ha encontrado inodo
        	
			memset(&(bloques_inodos[array[0]].inodos[array[1]]), 0, sizeof(TipoInodoDisco) ); // Rellenamos a 0
			
			return array; 

		}

	}

	// No se ha encontrado ninguna posición libre
	int *error = malloc (sizeof(int)*2);
	error[0] = -1;
	error[1] = -1;
	return error;

}

/*
 * @brief 	Allocates a block
 * @return 	block if success, -1 otherwise.
 */
int alloc(void)
{
	char b[BLOCK_SIZE];
	int i;

	for(i = 0; i<superbloque[0].numBloquesDatos; i++){  //Recorremos todos los bloques de de datos

		if(b_map[i] == 0){              // Primer bloque libre, indicado por el mapa

			b_map[i] = 1;               // Bloque ocupado ahora
			memset(b, 0, BLOCK_SIZE);   // Rellenamos a 0 el bloque
			bwrite(DEVICE_IMAGE, superbloque[0].primerBloqueDatos + i, b); // Lo grabamos a disco

			return i;
		}
	}

	return -1;
}

/*
 * @brief 	Liberates an inode
 * @return  1 if success, -1 if fail
 */
int ifree(int * arrayPosicion)
{	
	// Comprobar validez inodo
	if ((arrayPosicion[0]+1)*arrayPosicion[1] > NUMINODO){
		return -1;
	}

	// Liberar inodo
	i_map[(arrayPosicion[0]+1)*arrayPosicion[1]] = 0;

	return 1;
}

/*
 * @brief 	Liberates a block
 * @return  1 if success, -1 if fail
 */
int bfree(unsigned int * arrayBloquesDirectos)
{	
	// Comprobamos la validez de los bloques apuntados para ver que no se salen de rango
	for(int i=0; i<NUMBER_DIRECT_BLOCKS; i++){
		if(arrayBloquesDirectos[i] > superbloque[0].numBloquesDatos){
			return -1;
		}
	}
	
	// Liberamos los bloquea en el mapa de bloques en uso
	for(int i=0; i<NUMBER_DIRECT_BLOCKS; i++){
		if(arrayBloquesDirectos[i] != -1){
			b_map[arrayBloquesDirectos[i]] = 0;
		}
	}
	
	return 1;
}

/*
 * @brief 	Search an inode looking at the name
 * @return 	array of position of the inode if success, [-1, -1] otherwise.
 */
int * namei(char *fileName)
{	

	int bloque_buscar = -1;
	int inodo_buscar = -1;
	int *array = malloc (sizeof(int)*2);

	// Buscamos el inodo con el nombre fileName y devolvemos qué inodo es
	for (int i=0; i<BLOCKS_FOR_INODES; i++) {
		for(int j=0; j<NUMBER_INODES_PER_BLOCK; j++){
			
        	if(strcmp(fileName, bloques_inodos[i].inodos[j].nombre) == 0){ 
				//Asignamos los valores del array a devolver
				bloque_buscar = i;
				inodo_buscar = j;
			} 
		}
    }
	
	// Control de errores en caso de no encontarlo
	array[0] = bloque_buscar;
	array[1] = inodo_buscar;

	return array;
	
}

/*
 * @brief 	Search the associated block given a file descriptor and an offset
 * @return 	block if success, -1 otherwise.
 */
int bmap(int inodo_id, int offset)
{
	int bloque_logico;

	// Comprobar validez de inodo_id
	if (inodo_id > superbloque[0].numInodos){
		return -1;
	}

	// Buscar bloque lógico de datos asociado
	bloque_logico = offset / BLOCK_SIZE;


	int *array = malloc (sizeof(int)*2);
	array = computePositionInodeMap(inodo_id); // Calculamos la posición dentro de nuestro sistema de ficheros

	// Devolver la referencia añ bloque directo para saber de cuál estamos hablando
	if (bloque_logico >= 0 || bloque_logico < NUMBER_DIRECT_BLOCKS){ // Devolveremos el bloque directo que se encuentre dentro de los límites
		return bloques_inodos[array[0]].inodos[array[1]].bloqueDirecto[bloque_logico];
	}else{
		return -1; // Error
	}
}




/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{	
	int b_id; 
	int *inodo_id;

	inodo_id = ialloc(); 		/*Función que nos identifica el primer inodo libre*/

	if(inodo_id[0] < 0 || inodo_id[1] < 0){
		return -2; // Fallo, no hay inodos libres
	}

	b_id = alloc();				/*Función que nos identifica el primer bloque libre*/

	if(b_id < 0){
		ifree(inodo_id); // Liberamos el inodo seleccionado anteriormente
		return -2;       // Fallo, no hay bloques libres
	}

	for (int i=0; i<BLOCKS_FOR_INODES; i++) {
		for(int j=0; j<NUMBER_INODES_PER_BLOCK; j++){
			// El fichero ya existe en el sistema si el nombre ya está en uso
        	if(strcmp(fileName, bloques_inodos[i].inodos[j].nombre) == 0){ return -1;} 
		}
    }

	strcpy(	bloques_inodos[inodo_id[0]].inodos[inodo_id[1]].nombre, fileName);

	// Apuntamos al bloque libre
	bloques_inodos[inodo_id[0]].inodos[inodo_id[1]].bloqueDirecto[0] = b_id;

	// Cambiamos la estructura auxiliar para indicar que está abierto y su posición
	inodosx[(inodo_id[0]+1)*inodo_id[1]].posicion = 0;
	inodosx[(inodo_id[0]+1)*inodo_id[1]].abierto = 1; //¿SEGURO QUE A 1?????

	return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	int *array = malloc (sizeof(int)*2);

	array = namei(fileName);

	if(array[0] < 0 || array[1] < 0){
		return -1; // Fallo, el fichero no existe en el sistema
	}

	bfree(bloques_inodos[array[0]].inodos[array[1]].bloqueDirecto);
	
	memset(&(bloques_inodos[array[0]].inodos[array[1]]), 0, sizeof(TipoInodoDisco) ); // Rellenamos a 0
	
	if(ifree(array) == -1){  // Liberamos el inodo
		return -2;
	}
	
	return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{	
	// FALTA COMPROBAR INTEGRIDAD UNA VEZ LA TENGAMOS ??

	int *inodo_id = malloc (sizeof(int)*2);

	// Buscar el inodo asociado al nombre
	inodo_id = namei(fileName);

	if(inodo_id[0] < 0 || inodo_id[1] < 0){
		return -1; // Fallo, el fichero no existe en el sistema
	}

	if(inodosx[(inodo_id[0]+1)*inodo_id[1]].abierto == 1){
		return -2; // Fallo, el fichero ya está abierto
	}

	// Iniciar sesion de trabajo
	inodosx[(inodo_id[0]+1)*inodo_id[1]].abierto = 1;
	inodosx[(inodo_id[0]+1)*inodo_id[1]].posicion = 0;

	// Devolvemos el número de inodo correspondiente al fichero, su descriptor
	return (inodo_id[0]+1)*inodo_id[1];
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{	
	// Comprobamos el descriptor
	if((fileDescriptor < 0) || (fileDescriptor > NUMINODO-1)){
		return -1;
	}
	
	// Terminar sesion de trabajo
	inodosx[fileDescriptor].abierto = 0;
	inodosx[fileDescriptor].posicion = 0;

	return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{	
	char b[BLOCK_SIZE]; // Reserva de espacio para lo que vamos a leer
	int b_id;

	int *array = malloc (sizeof(int)*2);
	array = computePositionInodeMap(fileDescriptor); // Calculamos la posición dentro de nuestro sistema de ficheros

	if(inodosx[fileDescriptor].posicion + numBytes > bloques_inodos[array[0]].inodos[array[1]].size){

		/*  Si el puntero al fichero más lo que hay que leer es ya mayor que lo que de verdad contiene el fichero,
			comprobaremos ya cuánto se puede leer como máximo */
		numBytes = bloques_inodos[array[0]].inodos[array[1]].size - inodosx[fileDescriptor].posicion; // Lo que aún puedo leer
		printf("NumBytes es: %i\n", numBytes);
		printf("La primera mierda es: %i\n", bloques_inodos[array[0]].inodos[array[1]].size);
		printf("La segunda mierda es: %i\n", inodosx[fileDescriptor].posicion);
	}
	printf("HOLA 1\n");

	if(numBytes == 0){
		return 0; // Devolvemos 0 porque el puntero de posición está al final del fichero
	}else if(numBytes < 0){
		return -1; // Error
	}
	int resto_size;
	
	do {
		//int bloque_id = inodosx[fileDescriptor].posicion/BLOCK_SIZE; // 2049/2048->bloque_id (1)
		int bloque_offset = inodosx[fileDescriptor].posicion%BLOCK_SIZE; //2049%2048->bloque_offset (1)
		int resto_leer_del_bloque = BLOCK_SIZE - bloque_offset; //2048 -1 
		resto_size = numBytes - resto_leer_del_bloque; // 2048 - 2047 -> resto_size
		int a_leer =  (resto_size <= 0) ? numBytes :resto_leer_del_bloque; //Esto es si se cumple pilla size y si no resto_size
		
		b_id = bmap(fileDescriptor, inodosx[fileDescriptor].posicion); // Saber cual es el bloque asociado. Dado un descriptor de fichero y un offset te devuelve el bloque asociado
		if(b_id == -1){
			return -1; // Control de errores. 
		}
		bread(DEVICE_IMAGE, b_id, b);
		memmove(buffer, b+bloque_offset, a_leer); // Mueve desde posición mas b, a_leer bytes a buffer
		inodosx[fileDescriptor].posicion += a_leer;
		buffer = (char *) buffer + a_leer;
	}while(resto_size > 0);
	printf("HOLA 2\n");
	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{	
	char b[BLOCK_SIZE]; // Reserva de espacio para lo que vamos a escribir
	int b_id;

	int *array = malloc (sizeof(int)*2);
	array = computePositionInodeMap(fileDescriptor); // Calculamos la posición dentro de nuestro sistema de ficheros

	if(inodosx[fileDescriptor].posicion + numBytes > bloques_inodos[array[0]].inodos[array[1]].size){

		/*  Si el puntero al fichero más lo que hay que escribir es ya mayor que lo que de verdad contiene el fichero,
			comprobaremos ya cuánto se puede escribir como máximo */
		numBytes = BLOCK_SIZE - inodosx[fileDescriptor].posicion; // Lo que aún puedo leer
	}
		printf("NumBytes es: %i\n", numBytes);
		printf("La primera mierda es: %i\n", bloques_inodos[array[0]].inodos[array[1]].size);
		printf("La segunda mierda es: %i\n", inodosx[fileDescriptor].posicion);
	if(numBytes == 0){
		return 0; // Devolvemos 0 porque el puntero de posición está al final del fichero
	}else if(numBytes < 0){
		return -1; // Error
	}

	int resto_size;
	
	do {
		//int bloque_id = inodosx[fileDescriptor].posicion/BLOCK_SIZE; // 2049/2048->bloque_id (1)
		int bloque_offset = inodosx[fileDescriptor].posicion%BLOCK_SIZE; //2049%2048->bloque_offset (1)
		int resto_leer_del_bloque = BLOCK_SIZE - bloque_offset; //2048 -1 
		resto_size = numBytes - resto_leer_del_bloque; // 2048 - 2047 -> resto_size
		int a_leer =  (resto_size <= 0) ? numBytes :resto_leer_del_bloque; //Esto es si se cumple pilla size y si no resto_size
		

		b_id = bmap(fileDescriptor, inodosx[fileDescriptor].posicion); // Saber cual es el bloque asociado. Dado un descriptor de fichero y un offset te devuelve el bloque asociado
		if(b_id == -1){
			return -1; // Control de errores. 
		}
		if(b_id == -3){ // El bloque directo aún no ha sido inicializado
			int block_allocated = alloc();
			int *array = malloc (sizeof(int)*2);
			array = computePositionInodeMap(block_allocated); // Calculamos la posición dentro de nuestro sistema de ficheros
			bloques_inodos[array[0]].inodos[array[1]].bloqueDirecto[0] = block_allocated; // ESTO NO SERÁ 0, SERÁ OTRA COSA
		}
		bread(DEVICE_IMAGE, b_id, b);
		memmove(b+bloque_offset, buffer, a_leer); // Mueve desde posición mas b, a_leer bytes a buffer
		bwrite(DEVICE_IMAGE, b_id, b);            // Hay que volver a escribir en disco
		inodosx[fileDescriptor].posicion += a_leer;
		buffer = (char *) buffer + a_leer;

	}while(resto_size > 0);

	return numBytes;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	/* Lo primero es tratar la referencia. Existen 3 casos
	   FS_SEEK_CUR = Posición actual del puntero de posición
	   FS_SEEK_BEGIN = Comienzo del fichero
	   FS_SEEK_END = Final del fichero */

	int *array = malloc (sizeof(int)*2);
	array = computePositionInodeMap(fileDescriptor); // Calculamos la posición dentro de nuestro sistema de ficheros

	if(whence == 0){ // Desplazar a FS_SEEK_BEGIN, comienzo del fichero
		inodosx[fileDescriptor].posicion = 0;
		return 0;

	} else if (whence == bloques_inodos[array[0]].inodos[array[1]].size - 1) { // Desplazar a FS_SEEK_END, fin del fichero
		inodosx[fileDescriptor].posicion = bloques_inodos[array[0]].inodos[array[1]].size - 1;
		return 0;

	} else if(whence == inodosx[fileDescriptor].posicion){ 	// Caso FS_SEEK_CUR
		
		if(inodosx[fileDescriptor].posicion + offset > bloques_inodos[array[0]].inodos[array[1]].size){

			/*  Si el puntero al fichero más lo que hay que desplazar es ya mayor que lo que de verdad contiene el fichero,
				se sale fuera de los límites y error */
			return -1;
		}

		// Nueva posición de posición
		inodosx[fileDescriptor].posicion = inodosx[fileDescriptor].posicion + offset; 
		return 0;

	} else { // Caso de error por no ser whence ninguna de las anteriores
		return -1;
	}
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
