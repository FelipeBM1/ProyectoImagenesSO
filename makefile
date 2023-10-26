all: Cliente Gestor

server: gestor.o usuario.h
	gcc -o gestor gestor.o

server.o: Gestor.c usuario.h
	gcc -c Gestor.c

client: cliente.o usuario.h
	gcc -o cliente cliente.o

client.o: Cliente.c usuario.h
	gcc -c Client.c