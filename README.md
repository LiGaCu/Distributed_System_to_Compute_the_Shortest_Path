# EE450 Socket Programming Project

## Author

Full name: Jiatong Li
USC ID: 1046533527

## Work description

In this project, I build a distributed system to calculate the shortest path and its length between two vertexes that the user gives.

The user uses *client* to type in requested information (map id, source vertex, destination vertex, filesize), and *client* sends it to *aws*. Then *aws* gives the requested information to serverA and serverB, who will search the map and vertex information in their separated data files. After serverA and serverB finish searching and give back map information or no found massage to aws, aws forward the map information and requested information to serverC or no found massage to client. After serverC receives the map information and requested information, it uses Dijkstra Algorithm to find the shortest path and calculates delays. Then serverC sends the results to aws, who will then forward them to client. 

To begin the system, please start serverA, serverB, serverC and aws first, then start client with the following command:

``./client [map id (char)] [source vertex index (int)] [destination vertex index (int)] [filesize (int)]``

## Source files

**map1.txt** and **map2.txt** files should be put in the same dictionary as this file.

There are five source files: 
* ***serverA.cpp***
* ***serverB.cpp***
* ***serverC.cpp***
* ***aws.cpp***
* ***client.cpp***

Each of them contains all the codes that each of executable files needs.

There is also a ***Makefile*** file, generating the executable files and running servers according to the input command.

To generate or update all executable files, use the following command:

``make``

or

``make all``

To removes all .o and executable files, use the following command:

``make clean``

To run one of the server (for example: serverA), use the following command:

``make serverA``

To generate or update a specific executable file (for example: serverA), use the following command:

``make serverA_``

## Message format

### Request information from client to aws

The following messages are stored in a successive storage space and sent to the aws from client

*map id (char)* + *source vertex index (int)* + *destination vertex index (int)* + *file size (int)*

### Request information from aws to serverA and serverB

The above massage format is untorched and forwarded to serverA and serverB via aws

*map id (char)* + *source vertex index (int)* + *destination vertex index (int)* + *file size (int)*

### Map information from serverA or serverB to aws

There are two possible situations, and the data format is different in two situation

#### all map id, source vertex index and destination vertex index are found

The first part's length is fixed and the second part's length is subject to the length of map fragment in the txt file.

*header "serverA\n" (char array)* + *original data in the txt file for a specific map fragment (char array)* for serverA

or

*header "serverB\n" (char array)* + *original data in the txt file for a specific map fragment (char array)* for serverB

#### map id, source vertex index or destination vertex index is not found

The first part's length is a fixed header and the second part's length is a char donating error information.

*header "serverA!" (char array)* + * specific error massage (char)*

The *specific error massage* is corresponded to the following:
* m : the map is not found
* s : source vertex is not found
* d : destination vertex is not found
* b : both source vertex and destination vertex are not found

### Map information from aws to serverC

This massage is only sent to serverC when all map id, source vertex index and destination vertex index are found.

*source vertex index (int)* + *destination vertex index (int)* + *file size (int)* + *map id (char)* + *original data in the txt file for a specific map fragment (char array)*

### Calculated results from serverC to aws

The following messages are stored in a successive storage space and sent to the aws from serverC.

*shortest path length (float)* + *Transmission Delay (float)* + *Propagation Delay (float)* + *shortest path rountin (int array)*

Note that the end of path in *shortest path rountin* is the denoted by the value 2333.

### Results from aws to client

There are two possible situations, and the data format is different in two situation

#### all map id, source vertex index and destination vertex index are found

*header y (char)* + *shortest path length (float)* + *Transmission Delay (float)* + *Propagation Delay (float)* + *shortest path rountin (int array)*

#### map id, source vertex index or destination vertex index is not found

*header ! (char)* + *specific error massage (char)*

The *specific error massage* is corresponded to the following:
* m : the map is not found
* s : source vertex is not found
* d : destination vertex is not found
* b : both source vertex and destination vertex are not found

## Test results

I test it under the circumstances *map, source vertex and destination vertex all exist*, *map does not exist* , *source vertex does not exist*, *destination vertex does not exist* and *both source vertex and destination vertex do not exist*.

The execuable files work well as the project discription requires.

## Reused code

The functions *Dijkstra()* and *recoverPath()* in *serverC.cpp* are adapted from the original functions *Dijkstra()* and *searchPath()* from https://www.cnblogs.com/simuhunluo/p/7469495.html.

Some socket initialization functions (as marked in .cpp files) are adapted from the snippets in Beej's tutorial.

All other codes are designed and written by myself, Jiatong Li.
