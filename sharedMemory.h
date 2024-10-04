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
    sharedMemory(const char* name_shm, string name_sem_1, string name_sem_2, int num_cols, int num_rows);

    // Destroy shared memory
    ~sharedMemory();

    // Create shared memory object
    bool createAll(); 

    // Open shared memory
    bool openAll();

    // Close shared memory
    bool closeAll();

    // Write a vector of ints
    bool write(vector<int> data_input);

    // Read a vector of ints
    bool read(vector<int> data_output);

    // Write a 2D array of floats
    bool write2D(vector<vector<float>> data_input);

    // Read a 2D array of floats
    bool read2D(vector<vector<float>>& data_output);

private:
    string shm_name;       // Name of shared memory
    size_t shm_size_float; // Size of shared memory for float
    size_t shm_size_int;   // Size of shared memory for float
    int shm_fd;            // File descriptor of shared memory object
    void* shm_ptr;         // Pointer to the mapped shared memory region
    string sem_name_1;     // Semaphore name
    string sem_name_2;     // Semaphore name
    sem_t* sem_ptr_1;      // Pointer to the mapped semaphore region
    sem_t* sem_ptr_2;      // Pointer to the mapped semaphore region
    int cols;              // Number of columns
    int rows;              // Number of rows (1 if 1D)
};

//=====================================================================================

// Implementations of sharedMemory methods

//=====================================================================================

// Initialize shared memory and semaphore
sharedMemory::sharedMemory(const char* name_shm, string name_sem_1, string name_sem_2, int num_cols, int num_rows)
    : shm_name(name_shm), shm_size_float(sizeof(float) * num_cols * num_rows), shm_size_int(sizeof(int) * num_cols), sem_name_1(name_sem_1), sem_name_2(name_sem_2), shm_fd(-1), shm_ptr(nullptr), sem_ptr_1(nullptr), sem_ptr_2(nullptr), rows(num_rows), cols(num_cols) {}

// Destroy shared memory and semaphore
sharedMemory::~sharedMemory()
{
    //close();
    shm_unlink(shm_name.c_str());
    sem_unlink(sem_name_1.c_str());
    sem_unlink(sem_name_2.c_str());
}

//=====================================================================================

// Create shared memory object
bool sharedMemory::createAll()
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


    // Creates shared memory
    shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);

    // Error handling
    if (shm_fd == -1)
    {
        cerr << "shm_open failed.\n";
        return false;
    }

    // Use correct size for int or float
    if (rows == 1)
    {
        // Resize the momory object to the size of the data
        ftruncate(shm_fd, shm_size_int);

        // Memory map the shared memory object
        shm_ptr = mmap(0, shm_size_int, PROT_WRITE, MAP_SHARED, shm_fd, 0);

        // Error handling
        if (shm_ptr == MAP_FAILED)
        {
            cerr << "mmap failed.\n";
            return false;
        }
    }

    else
    {
        // Resize the memory object to the size of the data
        ftruncate(shm_fd, shm_size_float);

        // Memory map the shared memory object
        shm_ptr = mmap(0, shm_size_float, PROT_WRITE, MAP_SHARED, shm_fd, 0);

        // Error handling
        if (shm_ptr == MAP_FAILED)
        {
            cerr << "mmap failed.\n";
            return false;
        }
    }
    //cout << "Shared memory created and ready.\n"; // Debugging

    return true;
} // end create

//=====================================================================================

// Open shared memory
bool sharedMemory::openAll()
{
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

    // Open shared memory
    shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
    if (shm_fd == -1)
    {
        cerr << "shm_open failed.\n";
        return false;
    }

    // Use correct size for int or float
    if (rows == 1)
    {
        // Memory map the shared memory object
        shm_ptr = mmap(0, shm_size_int, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED)
        {
            cerr << "mmap failed: " << strerror(errno) << '\n';
            close(shm_fd);  // Ensure file descriptor is closed on failure
            return false;
        }
    }

    else
    {
        // Memory map the shared memory object
        shm_ptr = mmap(0, shm_size_float, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED)
        {
            cerr << "mmap failed: " << strerror(errno) << '\n';
            close(shm_fd);  // Ensure file descriptor is closed on failure
            return false;
        }        
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

    // Use correct size for int or float
    if (rows == 1)
    {
        munmap(shm_ptr, shm_size_int);
    }

    else
    {
        munmap(shm_ptr, shm_size_float);
    }
    
    close(shm_fd);
    return true;
} // end read

//=====================================================================================

// Write a vector of ints
bool sharedMemory::write(vector<int> data_input)
{
    // Write data to shared memory
    memcpy(shm_ptr, data_input.data(), shm_size_int);
    //cout << "Data written to shared memory\n"; // Debugging

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
bool sharedMemory::read(vector<int> data_output)
{
    // Wait until data is written
    if (sem_wait(sem_ptr_1) < 0) 
    {
        perror("sem_wait failed.");
        return false;
    }
    //cout << "Reading data.\n"; // Debugging

    // Read data from the shared memory
    if (shm_ptr == nullptr) 
    {
        cerr << "Shared memory pointer is null.\n";
        return false;
    }

    // Read from shared memory and write to output
    memcpy(data_output.data(), shm_ptr, shm_size_int);

    // Signal when done reading
    if (sem_post(sem_ptr_2) < 0) 
    {
        perror("sem_post failed.");
        return false;
    }
    //cout << "Done reading.\n"; // Debugging

    return true;
} // end read

//=====================================================================================

// Write a 2D array of floats
bool sharedMemory::write2D(vector<vector<float>> data_input)
{
    // Flatten the 2D vector into a 1D array
    vector<float> flattened_data;
    for (const auto& row : data_input)
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
    memcpy(shm_ptr, flattened_data.data(), shm_size_float);
    //cout << "Data written to shared memory\n"; // Debugging

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
} // end write2D

//=====================================================================================

// Read a 2D array of floats
bool sharedMemory::read2D(vector<vector<float>>& data_output)
{
    if (sem_wait(sem_ptr_1) < 0) 
    {
        perror("sem_wait failed.");
        return false;
    }
    //cout << "Reading data.\n"; // Debugging

    // Ensure output size matches the number of columns and rows
    data_output.resize(cols, vector<float>(rows));

    // Read data from shared memory and reshape it into the 2D array
    vector<float> flattened_data(cols * rows);

    // Copy the data from shared memory into the flattened vector
    memcpy(flattened_data.data(), shm_ptr, shm_size_float);

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
    for (int i = 0; i < cols; ++i)
    {
        for (int j = 0; j < rows; ++j)
        {
            data_output[i][j] = flattened_data[i * rows + j];
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

    return true;
} // end read2D

//=====================================================================================

#endif