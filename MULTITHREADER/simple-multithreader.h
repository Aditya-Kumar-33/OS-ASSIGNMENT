#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <chrono>

using namespace std;

// defining struct for using parallel_for
// this will be used to run single iteration
typedef struct{
  int low;
  int high;
  // int arr[high-low];
  function<void(int)> lambda;
  // std::function is like a box or container in C++ 
  // that can hold something callable, such as a function or a lambda.
  // This part specifies what kind of thing the box (std::function) can hold.
  //  It's saying that whatever goes into this box should be a callable thing (like a function or lambda), and that callable thing should take an int as a parameter and return void.
  //This is the name of the box. 
  
}thread_args;


// this will be used in 2d parallel_for
typedef struct{
  int low;
  int high;
  // low and  high will be used in outer loop
  int low2;
  int high2;
  // low2 and high2 will be used in inner loop

  
  function<void(int,int)> lambda;
  // std::function is like a box or container in C++ that can hold something callable,
  //  such as a function or a lambda.
  // This part specifies what kind of thing the box (std::function) can hold. 
  // It's saying that whatever goes into this box should be a callable thing (like a function or lambda), and that callable thing should take an int as a parameter and return void.
  //This is the name of the box. 
  
}thread_args2;


void *thread_func(void* ptr){
  thread_args* t=((thread_args*) ptr);
  
  for(int i=t->low;i<t->high;i++ ){
    
    if(i>=t->high){
      break;
    }
    t->lambda(i);
    // calling the function to execute
  

  }
  // pthread_exit(NULL);
  return NULL;
  
}

// function used in 2d parallel_for
void *thread_func2(void* ptr){
  thread_args2* t=((thread_args2*) ptr);
  for(int i=t->low;i<t->high;i++){
    // running outerloop

    for(int j=t->low2;j<t->high2;j++){
      // running innerlooop
      
      t->lambda(i,j);
      // calling function with two arguements
    }
  }
  
  
  
  return NULL;
  
}




void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads){
  // Record the start time
    auto start = std::chrono::high_resolution_clock::now();
  
  pthread_t tid[numThreads];//array to store id for threads
  thread_args args[numThreads];//to store arguements to be passed in function
  int chunk = (high-low)/numThreads;
  for(int i=0;i<numThreads;i++){
    args[i].low=i*chunk; 
    if(i==numThreads-1){//handles case when there are odd no. of threads
      // args->high=high;
      args[i].high=high;
    }
    else{
      args[i].high=(i+1)*chunk;

    }
    // args[i].high=(i+1)*chunk;
    args[i].lambda=lambda;//assigning lambda function to each arguement

    
    int createThreadResult = pthread_create(&tid[i], NULL, thread_func, (void*)&args[i]);
    // error handling
    if (createThreadResult != 0) {
        cerr << "Error creating thread: " << strerror(createThreadResult) << endl;
        // Handle the error in thread creation
    }
  
    

  }
  
  for (int i=0; i<numThreads; i++) {
    int joinThreadResult = pthread_join(tid[i], NULL);
    if (joinThreadResult != 0) {
        cerr << "Error joining thread: " << strerror(joinThreadResult) << endl;
        // Handling the error if we encountered some error in joining thread
        return; 
    }
      // // waiting for all the threads to finish executing

}
auto end = std::chrono::high_resolution_clock::now();
// Calculate the duration
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
cout<<"------------------------------------------------------"<<endl;
std::cout << "Execution Time 1D parallel_for: " << duration.count() << " microseconds" << std::endl;
cout<<"------------------------------------------------------"<<endl;
return;

  
}


// parallel_for for 2dimensional
void parallel_for(int low1, int high1,  int low2, int high2, 
         std::function<void(int, int)>  &&lambda, int numThreads){
           // Record the start time
    auto start = std::chrono::high_resolution_clock::now();



  pthread_t tid[numThreads];//array to store id for threads
  thread_args2 args[numThreads];//to store arguements to be passed in function
  int chunk = (high1-low1)/numThreads;
  for(int i=0;i<numThreads;i++){
    args[i].low=i*chunk; 
     if(i==numThreads-1){//handles case when there are odd no. of threads
      args[i].high=high1;
    }
    else{
      args[i].high=(i+1)*chunk;

    }
    
    // args[i].high=(i+1)*chunk;
    args[i].low2=low2;
    args[i].high2=high2;
    args[i].lambda=lambda;//assigning lambda function to each  arguement


    
    int createThreadResult = pthread_create(&tid[i], NULL, thread_func2, (void*)&args[i]);
    if (createThreadResult != 0) {
        cerr << "Error creating thread: " << strerror(createThreadResult) << endl;
        // Handling  the error in thread creation
    }
    // cout<<"created thread number: "<<i+1<<endl;

  }
  for (int i=0; i<numThreads; i++) {
    int joinThreadResult = pthread_join(tid[i], NULL);
    if (joinThreadResult != 0) {
        cerr << "Error joining thread: " << strerror(joinThreadResult) << endl;
        // Handling the error if we encountered some error in joining thread
        return;
    }


}
auto end = std::chrono::high_resolution_clock::now();
// Calculate the duration
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
cout<<"------------------------------------------------------"<<endl;
std::cout << "Execution Time: 2D parallel_for " << duration.count() << " microseconds" << std::endl;
cout<<"------------------------------------------------------"<<endl;
return;


         

}


int user_main(int argc, char **argv); 

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main


