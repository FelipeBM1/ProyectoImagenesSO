#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "Usuario.h"
#include "Tweet.h"
#include "Tweet_pendiente.h"

int AperturaPipeEscritura(char *pipe);

int main(int argc,char *  *argv)
{
    //variables para leer archivo
    int num = atoi(argv[2]);//Pasamos como int el numero de usuarios que tendra el gestor
    int usuarios[num][num];//creamos una matriz donde se guarda la informacion del archivo
    int i,j;//variables de recorrido del for
    int filas=0;//Contador de filas,que aumentara por cada linea que tenga el archivo
    char linea[200];//Arreglo que guarda la linea para que el contador pueda hacer su función
    char modo[1];//Declaracion del modo
    strcpy(modo,argv[6]);//paso del argumento al modo
    int tipo;
    //Variables para el pipe
    int fd, fd1,p;
    user u;
    seg solicitud;
    tw TW;
    int bn,respuesta,opcion;
    char pipe_p[10];
    mode_t fifo_mode = S_IRUSR|S_IWUSR;
    TP *Pendiente;
    int tamano_dinamico=0;
    int id_solicitado;

    //Verificar argumentos
    if(argc!=11)
    {
        printf("Formato debe ser $./gestor -n Num -r Relaciones -m modo -t time -p pipeNom");
        exit(1);
    }
    // limitar la invocación del gestor a solo dos modos
    if ((modo[0]!='A')&&(modo[0]!='D'))
    {
        printf("Error en la seleccion del modo: 'A' Para acoplado o 'D' para desacoplado\n");
        exit(1);
    }

    if(modo[0]=='A')
    {
        tipo=1;
    }
    else
    {
        tipo=2;
    }

    FILE *relaciones = fopen(argv[4],"r");//Apertura del Archivo

    if (relaciones == NULL)
    {
        printf("El archivo no pudo abrirse, vuelva a Intentarlo");
        exit(1);
    }
    int columna=0;
    while (!feof(relaciones))
    {
        fgets(linea,200,relaciones);//Contador de filas, para determinar el tamaño del archivo
        for(i=0; i<200; i++)
        {
            if(linea[i]==48 || linea[i]==49)
            {
                columna++;
            }
            if(linea[i]==10)
            {
                break;
            }
        }

        if(columna!=num)
        {
            printf("\nLa relacion ingresada,no cumple con los requisitos revise datos en la estructura o argumentos suministrados");
            exit(1);
        }
        columna=0;
        filas++;
    }
    fclose(relaciones);

    i=0;

    FILE *relaciones_2 = fopen(argv[4],"r");//Apertura del Archivo
    if (relaciones_2 == NULL)
    {
        printf("El archivo no pudo abrirse, vuelva a Intentarlo");
        exit(1);
    }

    while(1)
    {
        for(j=0; j<filas; j++)
        {
            fscanf(relaciones_2,"%i ",&usuarios[i][j]);//Guardado da las relaciones en una matriz local
            if(i==j && usuarios[i][j]==1)
            {
                printf("\nLa relacion ingresada presenta un error al tener un usuario que se sigue asi mismo");
                exit(1);
            }
        }
        fscanf(relaciones_2,"\n");
        i++;

        if(i==num)
        {
            break;
        }
    }

    fclose(relaciones_2);

    printf("\n Matriz ingresada:");

    for(i=0; i<num; i++)
    {
        printf("\n Usuario %i:",i+1);
        for(j=0; j<num; j++)
        {
            printf("%i ",usuarios[i][j]);
        }
    }

    user users[num];


    for(i=0; i<num; i++)
    {
        users[i].id =i+1;
        users[i].enLinea = 0;
        sprintf(users[i].pipe, "pipe%d", users[i].id);//Guardado de estado offline, a todos los usuarios disponibles
    }

    unlink(argv[10]);//Se elimina la pipe central en caso de ya existir

    //creacion del pipe
    if (mkfifo(argv[10], fifo_mode) == -1)
    {
        perror("Server mkfifo");
        exit(1);
    }
    printf("\n Apertura de la pipe central, se empezaran a recibir peticiones \n");
    fd = open(argv[10],O_RDONLY);//Apertura del Pipe Central

    while(1)
    {
        p=read(fd,&opcion,sizeof(int)); //lectura de opcion mandanda por cliente
        if (p==0)
        {
            continue;
        }
        printf("\nSe recibio una opcion %i",opcion);
        if(opcion==0)//login
        {
            p= read (fd, &u, sizeof(u));//Lectura de la estrucutra de Usuario
            if (p==-1)
            {
                perror("proceso lector:");
                exit(1);
            }
            else if(p==0)
            {
                continue;
            }
            bn = 0;
            for(int i=0; i<num; i++)
            {
                //verificar que el usuario exista y no este en linea
                if(users[i].id == u.id && users[i].enLinea == 0)
                {
                    users[i].enLinea =u.enLinea;
                    strcpy(users[i].pipe, u.pipe);
                    bn=1;
                    break;
                }
            }
            fd1 = AperturaPipeEscritura(u.pipe); //Apertura del pipe especifico para escritura
            if(bn)
            {
                printf("\nMandando respuesta existosa a usuario %i\n",u.id);
                respuesta = 1;
                write (fd1, &respuesta, sizeof(respuesta));//Envio Exitoso de login
                write(fd1,&tipo,sizeof(int));
            }
            else
            {
                printf("\nMandando solcitud errada a usuario %i\n",u.id);
                respuesta = 0;
                write (fd1, &respuesta, sizeof(respuesta));//Envio rechazado de login
            }
            close(fd1);

        }
        else if(opcion==1) //Seguir
        {
            p = read (fd,&solicitud,sizeof(solicitud));//Lectura de la estructura de Usuario
            if (p == -1)
            {
                perror("proceso lector:");
                exit(1);
            }
            else if(p==0)
            {
                continue;
            }

            for(int i=0; i<num; i++)
            {
                if(users[i].id==solicitud.id_o)//Busqueda de informacion del cliente en gestor
                {
                    strcpy(pipe_p,users[i].pipe);//Se encuentra la Pipe a la cual responderemos la solicitud
                    break;
                }
            }
            int valido=1;
            if(usuarios[solicitud.id_o-1][solicitud.id_p-1]==1)//Se busca en la matriz la solicitud para comprabar si es valida o no
            {
                valido=0;
            }

            if(valido==1)//de ser valida
            {
                printf("\nSe acepta la solcitud de seguir al usuario %i proveniente del usuario %i\n",solicitud.id_p,solicitud.id_o);
                usuarios[solicitud.id_o-1][solicitud.id_p-1]=1;//se cambia el estatus 0 a 1
                char Respuesta[20]="Solicitud Exitosa";//Se envia mensaje de exito
                fd1 = AperturaPipeEscritura(pipe_p); //Apertura del pipe especifico para escritura
                write (fd1,Respuesta,20);//envio
                printf("\n Matriz Actualizada:");

                for(i=0; i<num; i++)
                {
                    printf("\n Usuario %i:",i+1);
                    for(j=0; j<num; j++)
                    {
                        printf("%i ",usuarios[i][j]);
                    }
                }
              printf("\n");
            }
            else if(valido==0)//de ser errada
            {
                printf("\nSe rechaza la solcitud de seguir al usuario %i proveniente del usuario %i\n",solicitud.id_p,solicitud.id_o);
                char Respuesta[20]="Solicitud Errada"; //Se envia mensaje de fallo
                fd1 = AperturaPipeEscritura(pipe_p); //Apertura del pipe especifico para escritura
                write (fd1,Respuesta,20);
            }
            close(fd1);
        }
        else if(opcion==2) //Dejar de Seguir
        {
            p = read (fd, &solicitud, sizeof(solicitud));//Lectura de la estructura de Usuario
            if (p == -1)
            {
                perror("proceso lector:");
                exit(1);
            }
            else if(p==0)
            {
                continue;
            }
            for(int i=0; i<num; i++)
            {
                if(users[i].id==solicitud.id_o)//Busqueda de informacion del cliente en gestor
                {
                    strcpy(pipe_p,users[i].pipe);//Se encuentra la Pipe a la cual responderemos la solicitud
                    break;
                }
            }
            int valido=1;
            if(usuarios[solicitud.id_o-1][solicitud.id_p-1]==0) //se valida que el usario que hace la solicitud no siga al solicitado
            {
                valido=0;
            }

            if(valido==1)//si es valida la solicitud
            {
                printf("\nSe Acepta la solcitud de dejar de seguir al usuario %i proveniente del usuario %i\n",solicitud.id_p,solicitud.id_o);
                usuarios[solicitud.id_o-1][solicitud.id_p-1]=0;//se cambia el valor a 0
                char Respuesta[20]="Solicitud Exitosa";//envio exitoso
                fd1 = AperturaPipeEscritura(pipe_p); //Apertura del pipe especifico para escritura
                write (fd1,Respuesta,20);//envio Funcional
                printf("\n Matriz Actualizada:");

                for(i=0; i<num; i++)
                {
                    printf("\n Usuario %i:",i+1);
                    for(j=0; j<num; j++)
                    {
                        printf("%i ",usuarios[i][j]);
                    }
                }
              printf("\n");
            }
            else if(valido==0)
            {
                printf("\nSe rechaza la solcitud de dejar de seguir al usuario %i proveniente del usuario %i\n",solicitud.id_p,solicitud.id_o);
                char Respuesta[20]="Solicitud Errada";
                fd1 = AperturaPipeEscritura(pipe_p); //Apertura del pipe especifico para escritura
                write (fd1,Respuesta,20);
            }
            close(fd1);
        }
        else if(opcion==3)
        {
            int con_e=0;
            p=read(fd,&TW,sizeof(TW));
            if (p == -1)
            {
                perror("proceso lector:");
                exit(1);
            }
            else if(p==0)
            {
                continue;
            }
            for(i=0; i<num; i++)
            {
                if(usuarios[i][TW.id-1]==1)
                {
                    if(tamano_dinamico==0)
                    {
                        Pendiente=(TP*)malloc((tamano_dinamico+1)*sizeof(TP));
                        if (Pendiente==NULL)
                        {
                            perror("Memoria no se a podido asignar");
                            exit(1);
                        }
                    }
                    else
                    {
                        Pendiente=(TP*)realloc(Pendiente,(tamano_dinamico+1)*sizeof(TP));
                        if (Pendiente==NULL)
                        {
                            perror("Memoria no se a podido asignar");
                            exit(1);
                        }
                    }
                    Pendiente[tamano_dinamico].id_o=TW.id;
                    Pendiente[tamano_dinamico].id_d=i+1;
                    strcpy(Pendiente[tamano_dinamico].Mensaje,TW.Mensaje);
                    tamano_dinamico++;
                    con_e++;
                }
            }
            for(int i=0; i<num; i++)
            {
                if(TW.id==users[i].id)
                {
                    printf("\n Vamos a enviarle a la pipe %s que envio su tweet a %i usuarios\n",users[i].pipe,con_e);
                    fd1=AperturaPipeEscritura(users[i].pipe);
                    write(fd1,&con_e,sizeof(int));
                    break;
                }
            }
            for(int i=0; i<tamano_dinamico; i++)
            {
                printf("\nEsta en pendiente el mensaje '%s'proveniente de %i hacia %i\n",Pendiente[i].Mensaje,Pendiente[i].id_o,Pendiente[i].id_d);
            }
        }
        else if(opcion==4)
        {
            int con_envios=0;
            int aux_con=0;
            TP Aux_TPS[tamano_dinamico];
            p=read(fd,&id_solicitado,sizeof(int));
            printf("\nSe recibio el ID %i \n",id_solicitado);
            for(i=0; i<tamano_dinamico; i++)
            {
                if(id_solicitado==Pendiente[i].id_d)
                {
                    con_envios++;
                }
            }
            printf("\nSe encontraron %i tweet\n",con_envios);
            for(int i=0; i<num; i++)
            {
                if(id_solicitado==users[i].id)
                {
                    fd1=AperturaPipeEscritura(users[i].pipe);
                    write(fd1,&con_envios,sizeof(int));
                    break;
                }
            }
            for(i=0; i<tamano_dinamico; i++)
            {
                if(id_solicitado==Pendiente[i].id_d)
                {
                    TW.id=Pendiente[i].id_o;
                    strcpy(TW.Mensaje,Pendiente[i].Mensaje);
                    write(fd1,&TW,sizeof(TW));
                }
                else
                {
                    Aux_TPS[aux_con].id_o=Pendiente[i].id_o;
                    Aux_TPS[aux_con].id_d=Pendiente[i].id_d;
                    strcpy(Aux_TPS[aux_con].Mensaje,Pendiente[i].Mensaje);
                    aux_con++;
                }
            }
            tamano_dinamico=tamano_dinamico-con_envios;
            for(int i=0; i<(tamano_dinamico); i++)
            {
                if(i==0)
                    {
                        Pendiente=(TP*)malloc((i+1)*sizeof(TP));
                        if (Pendiente==NULL)
                        {
                            perror("Memoria no se a podido asignar");
                            exit(1);
                        }
                    }
                    else
                    {
                        Pendiente=(TP*)realloc(Pendiente,(i+1)*sizeof(TP));
                        if (Pendiente==NULL)
                        {
                            perror("Memoria no se a podido asignar");
                            exit(1);
                        }
                    }
                    Pendiente[i].id_o=Aux_TPS[i].id_o;
                    Pendiente[i].id_d=Aux_TPS[i].id_d;;
                    strcpy(Pendiente[i].Mensaje,Aux_TPS[i].Mensaje);
            }
        }
        else if(opcion==5) //Desconectarse
        {
            p = read (fd, &u, sizeof(u));//Lectura de la estructura de Usuario
            if (p == -1)
            {
                perror("proceso lector:");
                exit(1);
            }
            else if(p==0)
            {
                continue;
            }
            for(int i=0; i<num; i++)
            {
                if(users[i].id == u.id)//Busqueda de informacion del cliente en gestor
                {
                    users[i].enLinea = u.enLinea; //actualizacion estado
                    break;
                }
            }
            char Respuesta[20]="Vuelva Pronto!";
            printf("\nMandando solicitud de desconexion exitoso a %i\n",u.id);
            fd1 = AperturaPipeEscritura(u.pipe); //Apertura del pipe especifico para escritura
            write (fd1,Respuesta,20);
            unlink(u.pipe);
        }
    }
}
/*Nombre: Abrir pipe
  Entradas: Nombre del pipe
  Objetivo: Abrir y escribir en el pipe
  Salida: El fd del pipe
*/
int AperturaPipeEscritura(char *pipe)
{

    int i, Apertura, fd;
    do
    {
        if ((fd = open(pipe, O_WRONLY))==-1)
        {
            perror("Gestor abriendo Pipe Escritura");
            printf("Se volvera a intentar despues\n");
            sleep(5);
        }
        else Apertura = 1;
    }
    while (Apertura == 0);

    return fd;
}

