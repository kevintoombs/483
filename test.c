#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>

#define TESTS 22 //this goes up to 2mb
#define ITTERATIONS 100
#define MAXSIZE (1 << TESTS) //this SHOULDN'T work, but the compiler deals with it up until ~25 tests

int main(int argc,char *argv[])
{
   int rank,p,i,j,k,tRecv;
   struct timeval t1,t2;
   char buf[MAXSIZE] = "";

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);

   if (rank == 0){
     printf("All procs sending to 0.\n\n");
     printf("STATS:\nprocs: %d\ntests: %d\nitterations: %d,\nmax msg size: %dbytes\n\n", p, TESTS, ITTERATIONS, MAXSIZE);
   }
   assert(p>=2);

   for (i = 1; i < p; i++){
     if (rank == 0) printf("Recieving from rank %d (%d/%d)\n", i,i,p-1);
     for (j = 0; j < TESTS; j++){
       if (rank == 0) gettimeofday(&t1,NULL);

       //ITERATION LOOP
       for (k = 0; k < ITTERATIONS; k++){
         if (rank == 0){
           MPI_Status status;
          		MPI_Recv(&buf,1<<j,MPI_CHAR,i,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
         }
         else{
           MPI_Send(&buf,1<<j,MPI_CHAR,0,0,MPI_COMM_WORLD);
         }
       }

       if (rank == 0) {
         gettimeofday(&t2,NULL);
         tRecv = (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000;
         printf("Test %d/%d (size: %d bytes, itterations: %d) took %d millisec.\n", j+1,TESTS,1<<j,ITTERATIONS,tRecv);
         //int wow = 1<<j;
         //printf("%d^%d=%i\n",2,j,wow);
       }
     }
     if (rank!=0)
      break;
   }
   printf("end: %d\n", rank);
   MPI_Finalize();

}
