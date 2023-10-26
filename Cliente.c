#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "Usuario.h"
#include "Tweet.h"


int Menu(int tipo);
int AperturaPipeEscritura(char *pipe);
int AperturaPipeLectura(char *pipe);

int main(int argc, char **argv)
{
    //variables
    int peticion=0; //Variable que indica que quiere hacer cliente, Default 0(login)
    int respuesta; //Variable enviada por gestor, donde 1=valido, 0=rechazado
    int opcion; //Control de menu
    char salida[20];//Mensaje de Salida enviado por gestor
    //variables pipes
    int  fd, fd1,p;//fd pipe central, fd1 pipe especifica
    mode_t fifo_mode = S_IRUSR | S_IWUSR;//Permitir a las pipes escribir y leer
    //variables estructuras
    user users;//Estructura donde se guarda info de login
    seg solicitud; //Estructura de envio de la solicitud
    tw TW;//Estructura twwet para enviar el mensaje enviado
    int tipo;
    int enviados;
    int cantidad;

    //verificar argumentos
    if(argc!=5)
    {
        printf("Argumentos incorrectos, formato correcto: $./Cliente -i ID -p pipeNom");
        exit(1);
    }
    //Envio de informacion para login
    fd = AperturaPipeEscritura(argv[4]);//Abrir pipe central para escribir
    //Guardado de informacion del cliente
    users.id = atoi(argv[2]); //Guardado de ID
    users.enLinea = 1; //Estatus en linea
    sprintf(users.pipe, "pipe%d", users.id); //Guardado de nombre de la pipe especifica
    unlink(users.pipe); //Se elimina la pipe, en caso de que ya exista

    if (mkfifo (users.pipe, fifo_mode) == -1) //creación pipe especifica
    {
        perror("Client  mkfifo");
        exit(1);
    }

    write(fd,&peticion,sizeof(int)); //Mandado de opcion login(0)
    write(fd,&users,sizeof(users));//mandado de estructura

    fd1 = AperturaPipeLectura(users.pipe); //Se abre la pipecentral para lectura
    read(fd1, &respuesta, sizeof(respuesta));//Lectura de la respuesta del gestor
    read(fd1,&tipo,sizeof(tipo));

    if(respuesta)//En caso de ser exitosa se despilega el menu
    {
                opcion=4;
                write(fd,&opcion,sizeof(int));
                write(fd,&users.id,sizeof(int));
                fd1 = AperturaPipeLectura(users.pipe);
                read(fd1,&cantidad,sizeof(int));
                printf("\nSe tienen %i Tweets sin leer",cantidad);
                printf("\n\n Te han enviado estos tweets mientras estabas desconectado:");
                for(int i=0;i<cantidad;i++)
                {
                  read(fd1,&TW,sizeof(TW));
                  printf("\n El Usuario %i Tuiteo: %s",TW.id,TW.Mensaje);
                }
        do
        {
          printf("\n\n\t\t\t Bienvenido Usuario %i",users.id);
          opcion=Menu(tipo);
          if(tipo==1 && opcion==5)
          {
            opcion=-1;
          }
          if(tipo==1 && opcion==4)
          {
            opcion=5;
          }
            switch(opcion)
            {
            case 1: //Seguir a alguien
                system("clear");
                solicitud.id_o=users.id;//se guarda el id como el id del origen de la solicitud "solicitud.id_o"
                printf("\n\n Digite id del usuario que desea seguir:");
                scanf("%i",&solicitud.id_p);//Se guarda el id al que se le realiza la peticion
                if(solicitud.id_o==solicitud.id_p)
                {
                  printf("\nNo te puedes seguir a ti mismo");
                  break;
                }

                write(fd,&opcion,sizeof(int));//Envio de la opcion "Opcion 1(seguir)"
                write(fd,&solicitud,sizeof(solicitud));//Envio de la estructura que incluye la peticion

                fd1 = AperturaPipeLectura(users.pipe);//Abrimos un pipe lectura con la respuesta
                p = read(fd1,salida,20);//Se lee el mensaje de salida
                if (p == -1)
                {
                    perror("proceso lector:");
                    exit(1);
                }
                printf("\n %s",salida);//Puede ser mensaje exitoso o fallido
                break;
            case 2://Dejar de seguir
                system("clear");
                solicitud.id_o=users.id;//se guarda el id como el id del origen de la solicitud "solicitud.id_o"
                printf("\n\n Digite id del usuario que desea dejar de seguir:");
                scanf("%i",&solicitud.id_p);//Se guarda el id al que se le realiza la peticion

                write(fd,&opcion,sizeof(int));//Envio de la opcion "Opcion 2(dejar de seguir)"
                write(fd,&solicitud, sizeof(solicitud));//Envio de la estructura que incluye la peticion

                fd1 = AperturaPipeLectura(users.pipe);//Abrimos un pipe lectura con la respuesta
                p = read(fd1,salida,20);//Se lee el mensaje de salida
                if (p == -1)
                {
                    perror("proceso lector:");
                    exit(1);
                }
                printf("\n %s",salida);//Puede ser mensaje exitoso o fallido
                break;
            case 3://Publicar tweet
                system("clear");
                printf("\n\n Digite el tweet que desea publicar: \n");
                fgetc(stdin);
                if(fgets(TW.Mensaje,202,stdin)==NULL)
                {
                  break;
                }
                if(strcspn(TW.Mensaje,"\n")==strlen(TW.Mensaje))
                {
                  int c;
                  while((c=fgetc(stdin))!=EOF && c!='\n')
                  {
                  }
                  printf("El Tweet ingresado es demasiado extenso");
                  break;
                }
                TW.id=users.id;
                write(fd,&opcion,sizeof(int));
                write(fd,&TW,sizeof(TW));
                fd1 = AperturaPipeLectura(users.pipe);//Abrimos un pipe lectura con la respuesta
                p = read(fd1,&enviados,sizeof(int));
                printf("Se han eviado tu tweet a tus %i seguidores",enviados);
                break;
            case 4://recuperar tweets
                system("clear");
                write(fd,&opcion,sizeof(int));
                write(fd,&users.id,sizeof(int));
                fd1 = AperturaPipeLectura(users.pipe);
                read(fd1,&cantidad,sizeof(int));
                printf("\nSe tienen %i Tweets sin leer",cantidad);
                printf("\n\n Te han enviado estos tweets mientras estabas conectado:");
                for(int i=0;i<cantidad;i++)
                {
                  read(fd1,&TW,sizeof(TW));
                  printf("\n El Usuario %i Tuiteo: %s",TW.id,TW.Mensaje);
                }
                break;
            case 5://Desconectarse
                users.enLinea=0;//Estado offline

                write(fd,&opcion,sizeof(int)); //Envio de opcion desconectar(4)
                write(fd, &users, sizeof(users));//Envio de estructura con nueva informacion

                fd1 = AperturaPipeLectura(users.pipe);//Se abre la pipe especifica para lectura

                p = read(fd1,salida,20);//Se lee el mensaje de salida
                if (p == -1)
                {
                    perror("proceso lector:");
                    exit(1);
                }
                printf("\n\n  %s",salida);
                exit(1);
                break;
            default:
                printf("\n\n\t OPCION INVALIDA! Intente de nuevo\n\n");
                break;
            }
        }
        while(1);
    }
    else
        printf("El proceso NO exitoso\n");

    return 0;
}

/*Nombre: Menu
  Entradas:
  Objetivo: Mostrar el menu de opciones para el usuario
  Salida: La opcion elegida por el usuario
*/
int Menu(int tipo)
{
    int opc;
    printf("\n\n\t\t-----------------------------------");
    printf("\n\t\t\tMENU PRINCIPAL");
    printf("\n\t\t-----------------------------------");
    printf("\n\t\t 1.Follow");
    printf("\n\t\t 2.Unfollow");
    printf("\n\t\t 3.Publicar un Tweet");
    if(tipo==1)
    {
    printf("\n\t\t 4.Desconectarse");
    }
    if(tipo==2)
    {
    printf("\n\t\t 4.recuperar tweets");
    printf("\n\t\t 5.Desconectarse");
    }
    printf("\n\n\t\t-----------------------------------");
    printf("\n\n\n\t\tDigite la opcion: ");
    scanf("%d", &opc);
    return opc;
}

/*Nombre: CrearPipe
  Entradas: Nombre del pipe
  Objetivo: Abrir y escribir en el pipe
  Salida: El fd del pipe
*/
int AperturaPipeEscritura(char *pipe)
{

    int i, Apertura, fd;
    do
    {
        fd = open(pipe, O_WRONLY);
        if (fd == -1)
        {
            perror("pipe");
            printf(" Se volvera a intentar despues\n");
            sleep(10);
        }
        else Apertura = 1;
    }
    while (Apertura == 0);

    return fd;
}
/*Nombre: CrearPipe
  Entradas: Nombre del pipe
  Objetivo: Abrir y leer el pipe
  Salida: El fd del pipe
*/
int AperturaPipeLectura(char *pipe)
{

    int i, Apertura, fd;
    do
    {
        fd = open(pipe, O_RDONLY);
        if (fd == -1)
        {
            perror("pipe");
            printf(" Se volvera a intentar despues\n");
        }
        else Apertura = 1;
    }
    while (Apertura == 0);
    return fd;
}
