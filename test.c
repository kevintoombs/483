#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>

#define TESTS 100
#define ITTERATIONS 100
#define MAXSIZE 2^TESTS

int main(int argc,char *argv[])
{
   int rank,p,i,j,k;
   struct timeval t1,t2;

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);

   if (rank == 0){
     printf("All procs sending to 0.\n");
     printf("p:%d, t:%d, i:%d.\n", p, TESTS, ITTERATIONS);
   }
   assert(p>=2);


   char buf[MAXSIZE] = "";

   for (i = 1; i++; i<5){
     if (rank == 0) printf("%d/%d\n", i,p);
     for (j = 0; j++; j<TESTS){
       //if (rank == 0) printf("%d\n", j);
       for (k = 0; k++; k<ITTERATIONS){
         if (rank == 0){
           //if (rank == 0) printf("%d\n", k);
           MPI_Status status;
          gettimeofday(&t1,NULL);
          		MPI_Recv(&buf,2^j,MPI_CHAR,i,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
              if (rank == 0) printf("%d\n", k);
              //printf("received message %s from rank %d;\n",buf,status.MPI_SOURCE);
          gettimeofday(&t2,NULL);
         }
         else{
           MPI_Send(&buf,2^j,MPI_CHAR,0,MPI_ANY_TAG,MPI_COMM_WORLD);
         }
       }
     }
   }

   MPI_Finalize();
}
