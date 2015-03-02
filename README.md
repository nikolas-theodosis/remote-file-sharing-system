# remote-file-sharing-system
remote file sharing between server and client

Makefile included to compile both server and client.  
Run the server first by typing:  
./dataServer -p **port** -s **thread_pool_size** -q **queue_size**

Then run the client by typing:  
./remoteClient -i **server_ip** -p **server_port** -d **directory**  
where the **directory** is the one that the client wants to get from the server's filesystem.

