#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <algorithm>

//User defined parameters
#define N 16 //n = rows && n = columns
#define G 1000 //generations
#define X 100  //display every X generations

//Useful - only have one set to 1, have other at 0
//overrides random generation with a preseeded structures, they don't work with
//some of the MPI timing though, but they'll still catch total times.
#define BLINKERTEST 0 //class GoL shape //requires p > 3
#define GLIDERTEST 0  //class GoL shape //requires n/p > 4

//other globals
#define BIGPRIME 68111 //unused
#define BIGPRIME2 4639108321  //rank seed upper bound

int TOTALINITIME = 0;
int TOTALRUNTIME = 0;
int TOTALDISTIME = 0;
int TOTALSWAPTIME = 0;
int TOTALSCATTERTIME = 0;

void GenerateInitialGoL(int rank, int p, int initRows[][N]);
void GenerateTestGoLA(int rank, int p, int initRows[][N]);
void GenerateTestGoLB(int rank, int p, int initRows[][N]);
void TradeAboveAndBelow(int rank, int p, int myRows[][N], int aboveMate, int belowMate);
int Wrap(int i, int p);
void DisplayTimers(int rank, int gens, int display);
void Simulate(int myRows[][N], int p, int rank);
void DisplayGoL(int rank, int myRows[][N], int p, int g);
void DisplayRow(int row[]);
int DetermineState(int i, int j, int myRows[][N], int rank);
int ParseArgs(int argc,char *argv[], int &size, int &gens, int &display, int rank);

int main(int argc,char *argv[])
{
  MPI_Init(&argc,&argv);
  int rank, p, aboveMate, belowMate;
  struct timeval t1,t2, t3, t4;
  int size = N, gens = G, display = X;


  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&p);
  assert((N/p)%1==0);
  gettimeofday(&t1,NULL);

  ParseArgs(argc, argv, size, gens, display, rank);

  int myRows [(N/p)+2][N];
  if (BLINKERTEST) GenerateTestGoLA(rank, p, myRows);
  else if (GLIDERTEST) GenerateTestGoLB(rank, p, myRows);
  else GenerateInitialGoL(rank, p, myRows);

  aboveMate = Wrap(rank-1, p);
  belowMate = Wrap(rank+1, p);

  gettimeofday(&t2,NULL);
  TOTALINITIME = (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000;

  printf("%d: hi, above/below %d/%d \n", rank,aboveMate,belowMate);

  gettimeofday(&t1,NULL);
  for (int g = 0; g<=gens; g++) {
    TradeAboveAndBelow(rank, p, myRows, aboveMate, belowMate);
    Simulate(myRows, p, rank);
    if (g%display==0) {
      gettimeofday(&t3,NULL);
      DisplayGoL(rank, myRows, p, g);
      gettimeofday(&t4,NULL);
      TOTALDISTIME += (t4.tv_sec-t3.tv_sec)*1000 + (t4.tv_usec-t3.tv_usec)/1000;
    }
  }
  gettimeofday(&t2,NULL);
  TOTALRUNTIME = (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000;


  if (rank ==0)printf("Procs:%i N:%i G:%i\n", p, N, gens);
  MPI_Barrier(MPI_COMM_WORLD);
  DisplayTimers(rank, gens, display);

  MPI_Finalize();
  return 0;
}

int ParseArgs(int argc,char *argv[], int &size, int &gens, int &display, int rank){
  if (argc > 1){
    size = atoi(argv[1]);
    if (rank == 0) printf("argv[1] (%i) used as n. n = rows. n = cols.\n", size);
    if (rank == 0) printf("or not. you can change n in line 10 of the code.\n");
    if (argc > 2) {
      gens = atoi(argv[2]);
      if (rank == 0) printf("argv[2] (%i) used as g. g = times simulation is run.\n", gens);
      if (argc > 3){
        display = atoi(argv[3]);
        if (rank == 0) printf("argv[3] (%i) used as x. every xth simulation is displayed.\n", display);
      }
    }
  }
}

void DisplayTimers(int rank, int gens, int display){
  printf("Rank=%d: took %dms to %s.\n", rank,TOTALRUNTIME-TOTALDISTIME,"run (excluding display)");
  printf("Rank=%d: giving %dms avg for %d gens.\n", rank,(TOTALRUNTIME-TOTALDISTIME)/gens,gens);

  printf("Rank=%d: had %dms avg for %d displays.\n", rank,(TOTALDISTIME)/(gens/display),(gens/display));

  printf("Rank=%d: took %dms to %s.\n", rank,TOTALINITIME,"init");
  printf("Rank=%d: took %dms to %s.\n", rank,TOTALRUNTIME,"run");
  printf("Rank=%d: took %dms to %s.\n", rank,TOTALDISTIME,"display");
  printf("Rank=%d: took %dms to %s.\n", rank,TOTALSWAPTIME,"swap rows ");
  printf("Rank=%d: took %dms to %s.\n", rank,TOTALSCATTERTIME,"scatter");
}

void TradeAboveAndBelow(int rank, int p, int myRows[][N], int aboveMate, int belowMate){
  struct timeval t1,t2;
  gettimeofday(&t1,NULL);
  MPI_Send(&myRows[1], N, MPI_INT, aboveMate, 0, MPI_COMM_WORLD);
  MPI_Send(&myRows[N/p], N, MPI_INT, belowMate, 0, MPI_COMM_WORLD);

  MPI_Status status1, status2;

  MPI_Recv(&myRows[0],N,MPI_INT, aboveMate,MPI_ANY_TAG, MPI_COMM_WORLD,&status1);
 	MPI_Recv(&myRows[N/p+1],N,MPI_INT, belowMate ,MPI_ANY_TAG, MPI_COMM_WORLD,&status2);
  gettimeofday(&t2,NULL);
  TOTALSWAPTIME += (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000;
}

void DisplayGoL(int rank, int myRows[][N], int p, int g){
  if (rank != 0){
    for (int i = 1; i < (N/p + 1); i++){
      int rowTag = (rank * (N/p)) + i - 1;
      MPI_Send(myRows[i], N, MPI_INT, 0, rowTag, MPI_COMM_WORLD);
    }
  }
  else if (rank==0){
    printf("Current state at generation: %i\n", g);

    int allRows [N][N];
    MPI_Status status;

    for (int i = N/p; i < N; i++){
      MPI_Recv(allRows[i],N,MPI_INT, MPI_ANY_SOURCE,i, MPI_COMM_WORLD,&status);
    }

    for (int i = 1; i < N/p + 1; i++){
      DisplayRow(myRows[i]);
    }
    for (int i = N/p; i < N; i++){
      DisplayRow(allRows[i]);
    }
  }
}

void Simulate(int myRows[][N], int p, int rank){
  int newRows[(N/p)+2][N];
  for (int i = 1; i<(N/p+1); i++){
    for (int j = 0; j<N; j++){
      newRows[i][j] = DetermineState(i, j, myRows, rank);
    }
  }

  std::copy(&newRows[0][0], &newRows[0][0]+(N)*(N/p+2), &myRows[0][0]);
}

int DetermineState(int i, int j, int myRows[][N], int rank){
  int livingNeighboors = 0;
  for (int k = 0; k<3; k++){
    livingNeighboors += myRows[i-1][Wrap(j-1+k, N)];
    if (k!=1) livingNeighboors += myRows[i][Wrap(j-1+k, N)];
    livingNeighboors += myRows[i+1][Wrap(j-1+k, N)];
  }

  if (livingNeighboors < 2 || livingNeighboors > 3) return 0;
  else if (livingNeighboors == 3) return 1;
  else if (livingNeighboors == 2) return myRows[i][j];
}

void GenerateInitialGoL(int rank, int p, int initRows[][N]) {
  long randoms[p];
  long myRandom;
  struct timeval t1,t2;

  if (rank == 0){
    srand (time(NULL));
    for (int i = 0; i < p; i++){
        randoms[i] = (rand() % (BIGPRIME2));
    }
  }

  gettimeofday(&t1,NULL);
  MPI_Scatter(randoms, 1, MPI_LONG, &myRandom, 1, MPI_LONG, 0, MPI_COMM_WORLD);
  gettimeofday(&t2,NULL);
  TOTALSCATTERTIME = (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000;

  srand (myRandom);

  for(int i = 1; i<(N/p+1); i++){
    for(int j = 0; j<N; j++){
      initRows[i][j] = rand() % 2;
    }
  }
}

void GenerateTestGoLB(int rank, int p, int initRows[][N]) {
  assert(N/p>=4);
  for(int i = 1; i<(N/p+1); i++){
    for(int j = 0; j<N; j++){
        if (i==1 && j==1) initRows[i][j] = 1;
        else if (i==2 && j==2) initRows[i][j] = 1;
        else if (i==3 && j<3) initRows[i][j] = 1;
        else initRows[i][j] = 0;
    }
  }
}

void GenerateTestGoLA(int rank, int p, int initRows[][N]) {
  assert(p>=3);
  for(int i = 1; i<(N/p+1); i++){
    for(int j = 0; j<N; j++){
      if (rank==3 && i==1 && j < 3) {
        initRows[i][j] = 1;
      }
      else initRows[i][j] = 0;
    }
  }
}

int Wrap(int i, int p){
  if (i == -1){
    return p - 1;
  } else if (i == p) {
    return 0;
  } else
    return i;
}

void DisplayRow(int row[]){
  for (int i = 0; i<N; i++){
    if (row[i] == 0) printf("- ");
      else printf("%i ", row[i]);
  }

  printf("\n");
}
