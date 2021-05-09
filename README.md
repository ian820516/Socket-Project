# Socket-Project-

This is a socket project from class.

How to run:
1. make file
2. open five terminals
3. create a map file call "map.txt"(see map_simple.txt, map_hard.txt)
5. run the program in below order
6. run ./scheduler first 
7. run ./hospitalA [location] [capacity] [occupancy]
8. run ./hospitalB [location] [capacity] [occupancy]
9. run ./hospitalC [location] [capacity] [occupancy]
10. run ./client [location]

Example with TestCase: "map_simple.txt"  
Console output of Scheduler:   

./scheduler
The Scheduler is up and running.
The Scheduler has received information from Hospital A: total capacity is 10 and initial occupancy is 8
The Scheduler has received information from Hospital B: total capacity is 8 and initial occupancy is 5
The Scheduler has received information from Hospital C: total capacity is 5 and initial occupancy is 3
The Scheduler has received client at location 2 from the client using TCP over port 34937
The Scheduler has sent client location to Hospital A using UDP over port 33937
The Scheduler has sent client location to Hospital B using UDP over port 33937
The Scheduler has sent client location tp Hospital C using UDP over port 33937
The Scheduler has received map information from Hospital A, the score = 0.001348 and the distance = 824.033889
The Scheduler has received map information from Hospital B, the score = 0.001039 and the distance = 1327.614392
The Scheduler has received map information from Hospital C, the score = 0.002127 and the distance = 671.638724
The scheduler has sent the result to client using TCP over port 34937
The scheduler has assigned Hospital C to the client
The scheduler has sent the result to Hospital C using UDP over port 33937

Console output of Hospital A: 

./hospitalA 0 10 8
Hospital A is up and running using UDP on port 30937.
Hospital A has total capacity 10 and initial occupancy 8
Hospital A has received input from client at location 2
Hospital A has capacity = 10, occupation = 8, availability = 0.200000
Hospital A has found the shortest path to client, distance = 824.033889
Hospital A has the score = 0.001348
Hospital A has sent score = 0.001348 and distance = 824.033889 to the scheduler

Console output of Hospital B: 

./hospitalB 18 8 5
Hospital B is up and running using UDP on port 31937.
Hospital B has total capacity 8 and initial occupancy 5
Hospital B has received input from client at location 2
Hospital B has capacity = 8, occupation = 5, availability = 0.375000
Hospital B has found the shortest path to client, distance = 1327.614392
Hospital B has the score = 0.001039
Hospital B has sent score = 0.001039 and distance = 1327.614392 to the scheduler

Console output of Hospital C: 

./hospitalC 9 5 3
Hospital C is up and running using UDP on port 32937.
Hospital C has total capacity 5 and initial occupancy 3
Hospital C has received input from client at location 2
Hospital C has capacity = 5, occupation = 3, availability = 0.400000
Hospital C has found the shortest path to client, distance = 671.638724
Hospital C has the score = 0.002127
Hospital C has sent score = 0.002127 and distance = 671.638724 to the scheduler
Hospital C has been assigned to a client, occupation is update to 4, availability is updated to 0.200000

Console output of Client

./client 2
The client is up and running
The client has sent query to Scheduler using TCP: client location 2
The client has received results from the scheduler: assigned to Hospital C
