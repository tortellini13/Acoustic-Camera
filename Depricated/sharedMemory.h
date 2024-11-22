#ifndef SHM_FUNCTIONS
#define SHM_FUNCTIONS

// Other libraries
#include <iostream>
#include "PARAMS.h"

// Required for shared memory and semaphore
#include <sys/mman.h>  // For mmap, shm_open
#include <sys/stat.h>  // For mode constants
#include <fcntl.h>     // For O_* constants
#include <unistd.h>    // For ftruncate
#include <cstring>     // For memcpy
#include <semaphore.h> // For semaphore

using namespace std;

//=====================================================================================

// Method definitions
class sharedMemory
{
public:

    // Initialize shared memory
    sharedMemory(const char* name_shm_1, const char* name_shm_2, string name_sem_1, string name_sem_2, int num_cols_1, int num_rows_1, int num_cols_2);

    // Destroy shared memory
    ~sharedMemory();

    // Create shared memory object
    bool shmStart1(); 

    // Open shared memory
    bool shmStart2();

    // Close shared memory
    bool closeAll();

    // Writes audio data then reads user configs
    bool handleshm1(float data_input_1[], int data_output_2[]);

    // Reads audio data then writes user configs
    bool handleshm2(float data_output_1[], int data_input_2[]);

private:
    // General for all shared memory
    string sem_name_1;     // Semaphore name
    string sem_name_2;     // Semaphore name
    sem_t* sem_ptr_1;      // Pointer to the mapped semaphore region
    sem_t* sem_ptr_2;      // Pointer to the mapped semaphore region

    // For audio data
    int shm_fd_1;          // File descriptor of shared memory object
    string shm_name_1;     // Name of shared memory
    size_t shm_size_float; // Size of shared memory for float
    void* shm_ptr_1;       // Pointer to the mapped shared memory region
    int cols_1;            // Number of columns
    int rows_1;            // Number of rows

    // For user configs
    int shm_fd_2;          // File descriptor of shared memory object
    string shm_name_2;     // Name of shared memory
    size_t shm_size_int;   // Size of shared memory for int
    void* shm_ptr_2;       // Pointer to the mapped shared memory region
    int cols_2;            // Number of columns
};

//=====================================================================================

// Implementations of sharedMemory methods

//=====================================================================================

// Initialize shared memory and semaphore
sharedMemory::sharedMemory(const char* name_shm_1, const char* name_shm_2, string name_sem_1, string name_sem_2, int num_cols_1, int num_rows_1, int num_cols_2)
    : shm_name_1(name_shm_1), 
    shm_name_2(name_shm_2), 
    shm_size_float(sizeof(float) * num_cols_1 * num_rows_1), 
    shm_size_int(sizeof(int) * num_cols_2), 
    sem_name_1(name_sem_1), sem_name_2(name_sem_2), 
    shm_fd_1(-1), 
    shm_fd_2(-1),
    shm_ptr_1(nullptr), 
    shm_ptr_2(nullptr),
    sem_ptr_1(nullptr), 
    sem_ptr_2(nullptr), 
    rows_1(num_rows_1), 
    cols_1(num_cols_1),
    cols_2(num_cols_2) {}

// Destroy shared memory and semaphore
sharedMemory::~sharedMemory()
{
    //close();
    shm_unlink(shm_name_1.c_str());
    shm_unlink(shm_name_2.c_str());
    sem_unlink(sem_name_1.c_str());
    sem_unlink(sem_name_2.c_str());
}

//=====================================================================================

// Close shared memory
bool sharedMemory::closeAll()
{
    sem_close(sem_ptr_1);
    sem_close(sem_ptr_2);

    munmap(shm_ptr_1, shm_size_float);
    munmap(shm_ptr_2, shm_size_int);
    
    close(shm_fd_1);
    close(shm_fd_2);

    return true;
} // end read

//=====================================================================================

// Starts and syncs shm and sem for Program 1
bool sharedMemory::shmStart1()
{
    /*
    - Create sem_1
    - Create sem_2
    - Create shm_1
    - Post sem_1
    - Wait sem_2
    - Open shm_2
    */

    // Unlink to clean up old semaphores
    sem_unlink(sem_name_1.c_str());
    sem_unlink(sem_name_2.c_str());

    // Create semaphores
    sem_ptr_1 = sem_open(sem_name_1.c_str(), O_CREAT | O_EXCL, 0666, 0);
    sem_ptr_2 = sem_open(sem_name_2.c_str(), O_CREAT | O_EXCL, 0666, 0);
    
    //cout << sem_ptr_1 << endl;
    //cout << sem_ptr_2 << endl;

    // Handles errors
    if (sem_ptr_1 == SEM_FAILED)
    {
        perror("sem_1 failed to open.\n");
        return false;
    }

    // Handles errors
    if (sem_ptr_2 == SEM_FAILED)
    {
        perror("sem_2 failed to open.\n");
        return false;
    }
    cout << "1. Semaphores created and ready.\n"; // Debugging   

    //=====================================================================================

    // Creates shared memory for audio data
    shm_fd_1 = shm_open(shm_name_1.c_str(), O_CREAT | O_RDWR, 0666);

    // Error handling
    if (shm_fd_1 == -1)
    {
        cerr << "1. shm_open_1 failed.\n";
        return false;
    }

    // Resize the memory object to the size of the data
    ftruncate(shm_fd_1, shm_size_float);

    // Memory map the shared memory object
    shm_ptr_1 = mmap(0, shm_size_float, PROT_WRITE, MAP_SHARED, shm_fd_1, 0);

    // Error handling
    if (shm_ptr_1 == MAP_FAILED)
    {
        cerr << "1. mmap_1 failed.\n";
        return false;
    }
    cout << "1. Shared memory_1 created.\n"; // Debugging

    //=====================================================================================

    // Post to sem when shm_1 is created
    if (sem_post(sem_ptr_1) < 0) 
    {
        perror("1. sem_post_1 failed.");
        return false;
    }

    // Wait for shm_2 to be created
    if (sem_wait(sem_ptr_2) < 0)
    {
        perror("1. sem_wait_2 failed.");
        return false;
    }

    //=====================================================================================

    // Open shared memory for user configs
    shm_fd_2 = shm_open(shm_name_2.c_str(), O_RDWR, 0666);
    if (shm_fd_2 == -1)
    {
        cerr << "2. shm_open2 failed.\n";
        return false;
    }

    // Memory map the shared memory object
    shm_ptr_2 = mmap(0, shm_size_int, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_2, 0);
    if (shm_ptr_2 == MAP_FAILED)
    {
        cerr << "1. mmap_2 failed: " << strerror(errno) << '\n';
        close(shm_fd_2);  // Ensure file descriptor is closed on failure
        return false;
    }        

    cout << "1. Shared memory_2 opened.\n"; // Debugging

    return true;
} // end shmStart1

//=====================================================================================

// Starts and syncs shm and sem for Program 2
bool sharedMemory::shmStart2()
{
    /*
    - Open sem_1 and sem_2 in loop
    - Wait sem_1
    - Open shm_1
    - Create shm_2
    - Post sem_2
    */
    // Open sem_1
    while (1)
    {
        sem_ptr_1 = sem_open(sem_name_1.c_str(), 0);
        
        if (sem_ptr_1 != SEM_FAILED)
        {
            cout << "2. Sem_1 opened.\n";
            break;
        }

        // Error handling
        if (errno == ENOENT)
        {
            cerr << "2. Semaphore_1 not yet created. Trying again...\n";
        }
        else
        {
            perror("2. sem_open_1 failed.");
            return false;
        }
    }   

    // Open sem_2
    while (1)
    {
        sem_ptr_2 = sem_open(sem_name_2.c_str(), 0);
        
        if (sem_ptr_2 != SEM_FAILED)
        {
            cout << "2. Sem_2 opened.\n";
            break;
        }

        // Error handling
        if (errno == ENOENT)
        {
            cerr << "2. Semaphore_2 not yet created. Trying again...\n";
        }
        else
        {
            perror("2. sem_open_2 failed.");
            return false;
        }
    }       
    cout << "2. Both semaphores opened.\n"; // Debugging

    //=====================================================================================

    // Wait for shm_1 to be created
    if (sem_wait(sem_ptr_1) < 0)
    {
        perror("2. sem_wait_1 failed.");
        return false;
    }
   
    //=====================================================================================

    // Open shared memory for audio data
    shm_fd_1 = shm_open(shm_name_1.c_str(), O_RDWR, 0666);
    if (shm_fd_1 == -1)
    {
        cerr << "2. shm_open_1 failed.\n";
        return false;
    }

    // Memory map the shared memory object
    shm_ptr_1 = mmap(0, shm_size_float, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_1, 0);
    if (shm_ptr_1 == MAP_FAILED)
    {
        cerr << "2. mmap_1 failed: " << strerror(errno) << '\n';
        close(shm_fd_1);  // Ensure file descriptor is closed on failure
        return false;
    }
    cout << "2. Shared memory opened.\n"; // Debugging

    //=====================================================================================

    // Creates shared memory
    shm_fd_2 = shm_open(shm_name_2.c_str(), O_CREAT | O_RDWR, 0666);

    // Error handling
    if (shm_fd_2 == -1)
    {
        cerr << "2. shm_open_2 failed.\n";
        return false;
    }

    // Resize the memory object to the size of the data
    ftruncate(shm_fd_2, shm_size_float);

    // Memory map the shared memory object
    shm_ptr_2 = mmap(0, shm_size_int, PROT_WRITE, MAP_SHARED, shm_fd_2, 0);

    // Error handling
    if (shm_ptr_2 == MAP_FAILED)
    {
        cerr << "2. mmap_2 failed.\n";
        return false;
    }

    //=====================================================================================

    // Post to sem when shm_2 is created
    if (sem_post(sem_ptr_2) < 0) 
    {
        perror("2. sem_post_2 failed.");
        return false;
    }

    return true;
} // end read

//=====================================================================================

// Handles writing audio data, reading user configs, and syncing
bool sharedMemory::handleshm1(float data_input_1[], int data_output_2[])
{
    /*
    - Write shm_1
    - Post sem_1
    - Wait sem_2
    - Read shm_2
    */

    /*
    // Debugging
    cout << "1. Data to write:\n";
    for (int i = 0; i < cols_1; i++)
    {
        for (int j = 0; j < rows_1; j++)
        cout << data_input_1[i * rows_1 + j] << " ";
    }
    cout << endl;
    */

    // Write audio data to shared memory
    memcpy(shm_ptr_1, data_input_1, shm_size_float);
    //cout << "1. Audio data written to shared memory\n"; // Debugging

    //=====================================================================================

    // Post to sem when shm_1 is written
    if (sem_post(sem_ptr_1) < 0) 
    {
        perror("1. sem_post_1 failed.");
        return false;
    }
    //cout << "1. sem_post_1 done.\n";

    // Wait for shm 2 to be written
    if (sem_wait(sem_ptr_2) < 0) 
    {
        perror("1. sem_wait_2 failed.");
        return false;
    }
    //cout << "1. sem_wait_2 done.\n";

    //=====================================================================================

    // Ensure data_output is correctly allocated
    if (data_output_2 == nullptr) 
    {
        cerr << "1. data_output_2 is null.\n";
        return false;
    }

    // Read data from the shared memory
    if (shm_ptr_2 == nullptr) 
    {
        cerr << "1. shm_ptr_2 is null.\n";
        return false;
    }

    // Read from shared memory and write to output
    memcpy(data_output_2, shm_ptr_2, shm_size_int);

    return true;
} // end write

//=====================================================================================

// Handles writing user configs, reading audio data, and syncing
bool sharedMemory::handleshm2(float data_output_1[],int data_input_2[])
{
    /*
    - Wait sem_1
    - Read shm_1
    - Write shm_2
    - Post sem_2
    */

    // Wait for shm 1 to be written
    if (sem_wait(sem_ptr_1) < 0) 
    {
        perror("2. sem_wait_1 failed.");
        return false;
    }
    //cout << "2. sem_wait_1 done.\n";

    //=====================================================================================

    // Copy the data from shared memory into the flattened vector
    memcpy(data_output_1, shm_ptr_1, shm_size_float);
    //cout << "2. memcpy done.\n";

    /*
    // Debugging
    cout << "Data read:\n";
    for (int i = 0; i < cols_1; i++)
    {
        for (int j = 0; j < rows_1; j++)
        {
            cout << flattened_data[i * rows_1 + j] << " ";
        }
    }
    cout << endl;
    */

    //=====================================================================================

    // Write data to shared memory
    memcpy(shm_ptr_2, data_input_2, shm_size_int);
    //cout << "2. Data written to shared memory\n"; // Debugging

    //=====================================================================================

    // Post when shm 2 is done writing
    if (sem_post(sem_ptr_2) < 0) 
    {
        perror("2. sem_post_2 failed.");
        return false;
    }
    //cout << "2. sem_post_2 done.\n";

    return true;
} // end read

//=====================================================================================

#endif