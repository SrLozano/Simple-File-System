# A simple file system 🤖🗂
Development of a file system in user space written in C. The project consists of the creation of the file system architecture, including all the necessary control structures (i-nodes, superblock, etc) in user space. Some code it's provided by the professors of our university.

**Authors of the project**: Mario Lozano Cortés and Enrique Lozano Cebriano 📚💻

## Run it ⚙️

Use the makefile 

```bash
make clean
make
./test
```

## The logic behind 🧠

Next, we will define the structures, data and algorithms on which the file system is based. Initially, we must carry out a study on the parameters handled by the system, in this way we will be able to have a global image and obtain a structure that works as a file system. Later, we will emphasize the differential factors that optimize the system. After the mentioned study we defined the following structure of disc, which we will happen to define next.

|Superblock| i-nodes 1 | i-nodes 2 | Data blocks|
|--|--|--|--|

As you can see in the image, our associated structures for the organization of the file system go through the following ones:

 - Superblock: It contains the information that characterizes the file system, its main function is to indicate the size of the different parts that define the FS. The attributes selected for it are: numMagic (magic number that identifies the file system), numBlocksMapInodes (number of blocks occupied by the i-node map), numBlocksMapData (number of blocks occupied by the data map), numInodes (number of i-nodes defined in the system), firstInode (defines the block in which the first i-node will be found), numBlocksData (number of data blocks to be used), firstBlockData (indicates the position of the first data block on the disk), tamDevice (size of the disk to be used) bitmaps of the i-nodes and blocks (you can find more information about this in the optimization section) and padding (useful information that makes the superblock occupy a complete block). Below is a brief description of the bitmaps involved.
 
- Bitmap of the i-nodes: Array containing 1 bit for each of the i-nodes in the system. A 0 implies that the i-node in question is not being used, while a 1 implies that it is in use.

- Bitmap of blocks: Array containing 1 bit for each of the data blocks found in the system. A 0 implies that the block in question is not being used, while a 1 implies that it is in use.

- I-node blocks: i-node blocks are associated with files, which can be located in more than one block each. Due to the characteristics of the file system in which we are working, we have chosen an organization of the same that occupies only 2 blocks. This optimization process will be described in section 2.2 Main optimizations to be reviewed in this document.

- Data blocks: These blocks contain the information of the files that are going to be stored in the system. These blocks are referenced by the i-nodes corresponding to each file. The number of existing blocks will depend on the size of the disk on which the file system is mounted.

It is also worth mentioning that although it is not stored on disk, we have the following auxiliary structure that contains support information when handling the file system information. This structure is:

- Auxiliary structure for i-nodes: The objective of this structure is to store information for each one of the i-nodes that are in the system so that the state of a file is easily found. A structure is defined for each file (i-node), the attributes of this structure are the read/write position of the file associated to the i-node and whether it is open or closed.
