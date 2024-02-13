This project is to focus on the concepts of eventual consistency and interprocess communication. 
The idea of eventual consistency is when an update is made in a distributed network, the update will eventually be reflected
on all points of the network. This combined with interprocess communication, the method of communicating between running
processes on a machine, were used to make this project. This project is meant to model the YouTube like button feature where
multiple different servers send likes to a main server which will eventaully display a consistent number of likes once it has caught up.
In order to accomplish this, I was tasked with using sockets since that is what real world applications use for network communications.
Since this project is not hosted or does not require any external connections, all transactions are done over the localhost address
"127.0.0.1". The program runs for a total of 5 minutes in order to display running concurrent processes actively communicating.

My thought process for this project started with setting up a parent process program that would fork 10 separate child processes,
called LikesServer, which would generate random numbers between 0-42 and send that random number over a random interval between 1-5 seconds.
NOTE: I generated a random interval for the LikesServers but kept that interval (Ex. random number is sent every 3 seconds from LikesServer2)
Once I wrote this part of the program, I tested it using print statements to make sure that all 10 of the LikesServers generated random numbers and sent 
over the interval specified. Then I shifted to the hardest part of the project, setting up the socket. Although it is not a lot of code, I spent more time
researching how to properly configure the socket to my needs than any other part of the project. Once I had the socket setup, I tested by sending the information
from the LikesServers to the socket and receiving in the primaryLikesServer. Once I was able to receive the data, I worked on the actual parsing of the data and
accumulating the total like counter of the program. Then, I shifted all of the print statements to log files.
NOTE: When using tail -f, here are the following sample commands

tail -f /tmp/ParentProcessStatus/log.txt
tail -f /tmp/PrimaryLikesServer/log.txt
tail -f /tmp/LikesServer[0-10]/log.txt

After I completed all of this, another key process to fix was handling any leaks and errors with Valgrind.
NOTE: If you want to run valgrind, use the command: valgrind ./parentProcess & valgrind ./primaryLikesServer

I ran into multiple issues during this project, mainly with setting up the socket and dealing with Valgrind. With the socket, trying to configure the socket
to my needs was difficult but there are tons of resources online that discuss sockets and interprocess communication (references and links are in code). With Valgrind, there was a lot of issues. First, Valgrind struggles with multiprocess testing. On that note, when I ran valgrind make run, it was causing issues that were out of my control. Also, many different issues including a issue with uninitialised bytes and the write call to the socket (later found out that you have to use memset to avoid this issue). With that being said, this project was crucial in learning about the importance of eventual consistency and interprocess communication.
