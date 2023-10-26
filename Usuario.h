#include<stdio.h>
#include<stdlib.h>

typedef struct Usarios
{
    int id;
    int enLinea;
    char pipe[20];
    //int *seguidos = (int*) malloc(cant*sizeof(int));    
} user;

typedef struct seguir
{
  int id_o;
  int id_p;
} seg;
