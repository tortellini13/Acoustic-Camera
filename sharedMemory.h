#ifndef SHM_FUNCTIONS
#define SHM_FUNCTIONS

// Other libraries
#include <iostream>
#include <vector>
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
    bool writeRead1(vector<vector<float>> data_input_1, vector<int> data_output_2);

    // Reads audio data then writes user configs
    bool readWrite2(vector<vector<float>> data_output_1, vector<int> data_input_2);

    // Write a 2D array of floats
    //bool write2D(vector<vector<float>> data_input);

    // Read a 2D array of floats
    //bool read2D(vector<vector<float>>& data_output);

private:
    // General for all shared memory
    int shm_fd_1;          // File descriptor of shared memory object for audio data
    int shm_fd_2;          // File descriptor of shared memory object for audio data
    string sem_name_1;     // Semaphore name
    string sem_name_2;     // Semaphore name
    sem_t* sem_ptr_1;      // Pointer to the mapped semaphore region
    sem_t* sem_ptr_2;      // Pointer to the mapped semaphore region

    // For audio data
    string shm_name_1;     // Name of shared memory
    size_t shm_size_float; // Size of shared memory for float
    void* shm_ptr_1;       // Pointer to the mapped shared memory region
    int cols_1;              // Number of columns
    int rows_1;              // Number of rows (1 if 1D)

    // For user configs
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

// Create shared memory object
bool sharedMemory::shmStart1()
{
    // Unlink to clean up old semaphores
    sem_unlink(sem_name_1.c_str());
    sem_unlink(sem_name_2.c_str());

    // Create semaphores
    sem_ptr_1 = sem_open(sem_name_1.c_str(), O_CREAT | O_EXCL, 0666, 0);
    sem_ptr_2 = sem_open(sem_name_2.c_str(), O_CREAT | O_EXCL, 0666, 0);
    
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
    //cout << "Semaphores created and ready.\n"; // Debugging

    //=====================================================================================

    // Creates shared memory for audio data
    shm_fd_1 = shm_open(shm_name_1.c_str(), O_CREAT | O_RDWR, 0666);

    // Error handling
    if (shm_fd_1 == -1)
    {
        cerr << "shm_open failed.\n";
        return false;
    }

    // Resize the memory object to the size of the data
    ftruncate(shm_fd_1, shm_size_float);

    // Memory map the shared memory object
    shm_ptr_1 = mmap(0, shm_size_float, PROT_WRITE, MAP_SHARED, shm_fd_1, 0);

    // Error handling
    if (shm_ptr_1 == MAP_FAILED)
    {
        cerr << "mmap failed.\n";
        return false;
    }
    //cout << "Shared memory created and ready in program 1.\n"; // Debugging

    //=====================================================================================

    // Open shared memory for user configs
    shm_fd_2 = shm_open(shm_name_2.c_str(), O_RDWR, 0666);
    if (shm_fd_2 == -1)
    {
        cerr << "shm_open failed.\n";
        return false;
    }

    // Memory map the shared memory object
    shm_ptr_2 = mmap(0, shm_size_int, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_2, 0);
    if (shm_ptr_2 == MAP_FAILED)
    {
        cerr << "mmap failed: " << strerror(errno) << '\n';
        close(shm_fd_2);  // Ensure file descriptor is closed on failure
        return false;
    }        

    //cout << "Shared memory opened.\n"; // Debugging

    return true;
} // end create

//=====================================================================================

// Open shared memory
bool sharedMemory::shmStart2()
{
    // Creates shared memory for user configs
    shm_fd_2 = shm_open(shm_name_2.c_str(), O_CREAT | O_RDWR, 0666);

    // Error handling
    if (shm_fd_2 == -1)
    {
        cerr << "shm_open failed.\n";
        return false;
    }

    // Resize the memory object to the size of the data
    ftruncate(shm_fd_2, shm_size_int);

    // Memory map the shared memory object
    shm_ptr_2 = mmap(0, shm_size_int, PROT_WRITE, MAP_SHARED, shm_fd_2, 0);

    // Error handling
    if (shm_ptr_2 == MAP_FAILED)
    {
        cerr << "mmap failed.\n";
        return false;
    }
    //cout << "Shared memory created and ready in program 1.\n"; // Debugging

    // Open sem_1
    while (1)
    {
        sem_ptr_1 = sem_open(sem_name_1.c_str(), 0);
        
        if (sem_ptr_1 != SEM_FAILED)
        {
            break;
        }

        // Error handling
        if (errno == ENOENT)
        {
            cerr << "Semaphore 1 not yet created. Trying again...\n";
        }
        else
        {
            perror("sem_open 1 failed.");
            return false;
        }
    }   

    // Open sem_2
    while (1)
    {
        sem_ptr_2 = sem_open(sem_name_2.c_str(), 0);
        
        if (sem_ptr_2 != SEM_FAILED)
        {
            break;
        }

        // Error handling
        if (errno == ENOENT)
        {
            cerr << "Semaphore 2 not yet created. Trying again...\n";
        }
        else
        {
            perror("sem_open 2 failed.");
            return false;
        }
    }       
    //cout << "Both semaphores opened.\n"; // Debugging

    // Open shared memory for audio data
    shm_fd_1 = shm_open(shm_name_1.c_str(), O_RDWR, 0666);
    if (shm_fd_1 == -1)
    {
        cerr << "shm_open failed.\n";
        return false;
    }

    // Memory map the shared memory object
    shm_ptr_1 = mmap(0, shm_size_float, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_1, 0);
    if (shm_ptr_1 == MAP_FAILED)
    {
        cerr << "mmap failed: " << strerror(errno) << '\n';
        close(shm_fd_1);  // Ensure file descriptor is closed on failure
        return false;
    }        

    //cout << "Shared memory opened.\n"; // Debugging

    return true;
} // end read

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

// Write a vector of ints
bool sharedMemory::writeRead1(vector<vector<float>> data_input_1, vector<int> data_output_2)
{
    // Write audio data
    // Flatten the 2D vector into a 1D array
    vector<float> flattened_data;
    for (const auto& row : data_input_1)
    {
        flattened_data.insert(flattened_data.end(), row.begin(), row.end());
    }

    /*
    // Debugging
    cout << "Flattened data write:\n";
    for (int i = 0; i < cols * rows; i++)
    {
        cout << flattened_data[i] << " ";
    }
    cout << endl;
    */

    // Write data to shared memory
    memcpy(shm_ptr_1, flattened_data.data(), shm_size_float);
    //cout << "Data written to shared memory\n"; // Debugging

    //=====================================================================================

    // Read user configs
    //cout << "Reading data.\n"; // Debugging

    // Read data from the shared memory
    if (shm_ptr_2 == nullptr) 
    {
        cerr << "Shared memory pointer is null.\n";
        return false;
    }

    // Read from shared memory and write to output
    memcpy(data_output_2.data(), shm_ptr_2, shm_size_int);

    //cout << "Done reading.\n"; // Debugging

    //=====================================================================================

    // Signal that data is done writing and is ready to be read
    if (sem_post(sem_ptr_1) < 0) 
    {
        perror("sem_post failed.");
        return false;
    }
    //cout << "Semaphore posted.\n"; // Debugging

    // Wait for response
    if (sem_wait(sem_ptr_2) < 0) 
    {
        perror("sem_wait failed.");
        return false;
    }
    //cout << "Response received.\n"; // Debugging

    return true;
} // end write

//=====================================================================================

// Read a vector of ints
bool sharedMemory::readWrite2(vector<vector<float>> data_output_1,vector<int> data_input_2)
{
    // Read audio data
    if (sem_wait(sem_ptr_1) < 0) 
    {
        perror("sem_wait failed.");
        return false;
    }
    //cout << "Reading data.\n"; // Debugging

    // Ensure output size matches the number of columns and rows
    data_output_1.resize(cols_1, vector<float>(rows_1));

    // Read data from shared memory and reshape it into the 2D array
    vector<float> flattened_data(cols_1 * rows_1);

    // Copy the data from shared memory into the flattened vector
    memcpy(flattened_data.data(), shm_ptr_1, shm_size_float);

    /*
    // Debugging
    cout << "Flattened data read:\n";
    for (int i = 0; i < cols * rows; i++)
    {
        cout << flattened_data[i] << " ";
    }
    cout << endl;
    */

    // Rebuild the 2D array from the flattened data
    for (int i = 0; i < cols_1; ++i)
    {
        for (int j = 0; j < rows_1; ++j)
        {
            data_output_1[i][j] = flattened_data[i * rows_1 + j];
        }
    }

    /*
    // Debugging
    cout << "Reconstructed data:\n";
    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < rows; j++)
        {
            cout << data_output[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
    */

    // Signal when done reading
    if (sem_post(sem_ptr_2) < 0) 
    {
        perror("sem_post failed.");
        return false;
    }
    //cout << "Done reading.\n"; // Debugging

    //=====================================================================================

    // Write user configs
    memcpy(shm_ptr_2, data_input_2.data(), shm_size_int);
    //cout << "Data written to shared memory\n"; // Debugging

    // Signal that data is done writing and is ready to be read
    if (sem_post(sem_ptr_1) < 0) 
    {
        perror("sem_post failed.");
        return false;
    }
    //cout << "Semaphore posted.\n"; // Debugging

    return true;
} // end read

//=====================================================================================

#endif