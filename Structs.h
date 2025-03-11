#pragma once

#include <iostream>
#include <complex>
#include <iomanip>

using namespace std;

//=====================================================================================

// Template struct for 2D arrays
template <typename T>
struct array2D
{
    T* data;         // Pointer to the data array
    size_t dim_1;    // First dimension (rows)
    size_t dim_2;    // Second dimension (columns)

    int* dim_1_index; // Index for the first dimension
                      // Second dimension does not need an index

    // Constructor
    array2D(size_t d1, size_t d2) : dim_1(d1), dim_2(d2)
    {
        // Allocate and zero-initialize the array
        data = new T[d1 * d2](); 

        // Allocate and zero-initialize the index arrays
        dim_1_index = new int[d1]();

        // Calculate the contribution of each dimension to the index
        for (int i = 0; i < d1; i++) {dim_1_index[i] = i * d2;}
    }

    // Destructor
    ~array2D()
    {
        delete[] data;
        delete[] dim_1_index;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j)
    {
        return data[dim_1_index[i] + j];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j) const
    {
        return data[dim_1_index[i] + j];
    }

    // Print array
    void print() const
    {
        for (size_t i = 0; i < dim_1; ++i)
        {
            for (size_t j = 0; j < dim_2; ++j)
            {
                cout << setw(8) << fixed << setprecision(6) << showpos << at(i, j) << " ";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    void fill(T value)
    {
        for (int i = 0; i < dim_1; i++)
        {
            for (int j = 0; j < dim_2; j++)
            {
                at(i, j) = value;
            }
        }
    }
}; // end array2D

//=====================================================================================

// Template struct for 3D arrays
template <typename T>
struct array3D
{
    T* data;         // Pointer to the data array
    size_t dim_1;    // First dimension (rows)
    size_t dim_2;    // Second dimension (columns)
    size_t dim_3;    // Third dimension (depth)

    int* dim_1_index; // Index for the first dimension
    int* dim_2_index; // Index for the second dimension
                      // Third dimension does not need an index

    // Constructor
    array3D(size_t d1, size_t d2, size_t d3) : dim_1(d1), dim_2(d2), dim_3(d3)
    {
        // Allocate and zero-initialize the array
        data = new T[d1 * d2 * d3](); 

        // Allocate and zero-initialize the index arrays
        dim_1_index = new int[d1]();
        dim_2_index = new int[d2]();

        // Calculate the contribution of each dimension to the index
        for (int i = 0; i < d1; i++) {dim_1_index[i] = i * d2 * d3;}
        for (int j = 0; j < d2; j++) {dim_2_index[j] = j * d3;}
    }

    // Destructor
    ~array3D()
    {
        delete[] data;
        delete[] dim_1_index;
        delete[] dim_2_index;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k)
    {
        return data[dim_1_index[i] + dim_2_index[j] + k];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k) const
    {
        return data[dim_1_index[i] + dim_2_index[j] + k];
    }

    // Print array
    void print() const
    {
        for (size_t i = 0; i < dim_1; ++i)
        {
            cout << "Layer " << i << ":" << endl;
            for (size_t j = 0; j < dim_2; ++j)
            {
                for (size_t k = 0; k < dim_3; ++k)
                {
                    cout << at(i, j, k) << " ";
                }
                cout << "\n";
            }
            cout << "\n";
        }
    }

    // Print array
    void print_layer(const int layer) const
    {
        for (size_t i = 0; i < dim_1; ++i)
        {
            for (size_t j = 0; j < dim_2; ++j)
            {
                cout << setw(8) << fixed << setprecision(6) << showpos << at(i, j, layer) << " ";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    void fill(T value)
    {
        for (int i = 0; i < dim_1; i++)
        {
            for (int j = 0; j < dim_2; j++)
            {
                for (int k = 0; k < dim_3; k++)
                {
                    at(i, j, k) = value;
                }
            }
        }
    }
}; // end array3D

//=====================================================================================

// Template struct for 4D arrays
template <typename T>
struct array4D
{
    T* data;         // Pointer to the data array
    size_t dim_1;    // First dimension
    size_t dim_2;    // Second dimension
    size_t dim_3;    // Third dimension
    size_t dim_4;    // Fourth dimension

    int* dim_1_index; // Index for the first dimension
    int* dim_2_index; // Index for the second dimension
    int* dim_3_index; // Index for the third dimension
                      // Fourth dimension does not need an index

    // Constructor
    array4D(size_t d1, size_t d2, size_t d3, size_t d4) 
        : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4)
    {
        // Allocate and zero-initialize the array
        data = new T[d1 * d2 * d3 * d4](); 

        // Allocate and zero-initialize the index arrays
        dim_1_index = new int[d1]();
        dim_2_index = new int[d2]();
        dim_3_index = new int[d3]();

        // Calculate the contribution of each dimension to the index
        for (int i = 0; i < d1; i++) {dim_1_index[i] = i * d2 * d3 * d4;}
        for (int j = 0; j < d2; j++) {dim_2_index[j] = j * d3 * d4;}
        for (int k = 0; k < d3; k++) {dim_3_index[k] = k * d4;}
    }

    // Destructor
    ~array4D()
    {
        delete[] data;
        delete[] dim_1_index;
        delete[] dim_2_index;
        delete[] dim_3_index;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k, size_t l)
    {
        return data[dim_1_index[i] + dim_2_index[j] + dim_3_index[k] + l];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k, size_t l) const
    {
        return data[dim_1_index[i] + dim_2_index[j] + dim_3_index[k] + l];
    }

    void fill(T value)
    {
        for (int i = 0; i < dim_1; i++)
        {
            for (int j = 0; j < dim_2; j++)
            {
                for (int k = 0; k < dim_3; k++)
                {
                    for (int l = 0; l < dim_4; l++)
                    {
                        at(i, j, k, l) = value;
                    }
                }
            }
        }
    }
}; // end array4D

//=====================================================================================

// Template struct for 5D arrays
template <typename T>
struct array5D
{
    T* data;         // Pointer to the data array
    size_t dim_1;    // First dimension
    size_t dim_2;    // Second dimension
    size_t dim_3;    // Third dimension
    size_t dim_4;    // Fourth dimension
    size_t dim_5;    // Fifth dimension

    int* dim_1_index; // Index for the first dimension
    int* dim_2_index; // Index for the second dimension
    int* dim_3_index; // Index for the third dimension
    int* dim_4_index; // Index for the fourth dimension
                      // Fifth dimension does not need an index

    // Constructor
    array5D(size_t d1, size_t d2, size_t d3, size_t d4, size_t d5) 
        : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4), dim_5(d5)
    {
        // Allocate and zero-initialize the array
        data = new T[d1 * d2 * d3 * d4 * d5](); 

        // Allocate and zero-initialize the index arrays
        dim_1_index = new int[d1]();
        dim_2_index = new int[d2]();
        dim_3_index = new int[d3]();
        dim_4_index = new int[d4]();

        // Calculate the contribution of each dimension to the index
        for (int i = 0; i < d1; i++) {dim_1_index[i] = i * d2 * d3 * d4 * d5;}
        for (int j = 0; j < d2; j++) {dim_2_index[j] = j * d3 * d4 * d5;}
        for (int k = 0; k < d3; k++) {dim_3_index[k] = k * d4 * d5;}
        for (int l = 0; l < d4; l++) {dim_4_index[l] = l * d5;}
    }

    // Destructor
    ~array5D()
    {
        delete[] data;
        delete[] dim_1_index;
        delete[] dim_2_index;
        delete[] dim_3_index;
        delete[] dim_4_index;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k, size_t l, size_t m)
    {
        return data[dim_1_index[i] + dim_2_index[j] + dim_3_index[k] + dim_4_index[l] + m];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k, size_t l, size_t m) const
    {
        return data[dim_1_index[i] + dim_2_index[j] + dim_3_index[k] + dim_4_index[l] + m];
    }

    void fill(T value)
    {
        for (int i = 0; i < dim_1; i++)
        {
            for (int j = 0; j < dim_2; j++)
            {
                for (int k = 0; k < dim_3; k++)
                {
                    for (int l = 0; l < dim_4; l++)
                    {
                        for (int m = 0; m < dim_5; m++)
                        {
                            at(i, j, k, l, m) = value;
                        }
                    }
                }
            }
        }
    }
}; // end array5D

//=====================================================================================

// Template struct for 6D arrays
template <typename T>
struct array6D
{
    T* data;         // Pointer to the data array
    size_t dim_1;    // First dimension
    size_t dim_2;    // Second dimension
    size_t dim_3;    // Third dimension
    size_t dim_4;    // Fourth dimension
    size_t dim_5;    // Fifth dimension
    size_t dim_6;    // Sixth dimension

    int* dim_1_index; // Index for the first dimension
    int* dim_2_index; // Index for the second dimension
    int* dim_3_index; // Index for the third dimension
    int* dim_4_index; // Index for the fourth dimension
    int* dim_5_index; // Index for the fifth dimension
                      // Sixth dimension does not need an index

    // Constructor
    array6D(size_t d1, size_t d2, size_t d3, size_t d4, size_t d5, size_t d6) 
        : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4), dim_5(d5), dim_6(d6)
    {
        // Allocate and zero-initialize the array
        data = new T[d1 * d2 * d3 * d4 * d5 * d6](); 

        // Allocate and zero-initialize the index arrays
        dim_1_index = new int[d1]();
        dim_2_index = new int[d2]();
        dim_3_index = new int[d3]();
        dim_4_index = new int[d4]();
        dim_5_index = new int[d5]();

        // Calculate the contribution of each dimension to the index
        for (int i = 0; i < d1; i++) {dim_1_index[i] = i * d2 * d3 * d4 * d5 * d6;}
        for (int j = 0; j < d2; j++) {dim_2_index[j] = j * d3 * d4 * d5 * d6;}
        for (int k = 0; k < d3; k++) {dim_3_index[k] = k * d4 * d5 * d6;}
        for (int l = 0; l < d4; l++) {dim_4_index[l] = l * d5 * d6;}
        for (int m = 0; m < d5; m++) {dim_5_index[m] = m * d6;}
    }

    // Destructor
    ~array6D()
    {
        delete[] data;
        delete[] dim_1_index;
        delete[] dim_2_index;
        delete[] dim_3_index;
        delete[] dim_4_index;
        delete[] dim_5_index;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k, size_t l, size_t m, size_t n)
    {
        return data[dim_1_index[i] + dim_2_index[j] + dim_3_index[k] + dim_4_index[l] + dim_5_index[m] + n];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k, size_t l, size_t m, size_t n) const
    {
        return data[dim_1_index[i] + dim_2_index[j] + dim_3_index[k] + dim_4_index[l] + dim_5_index[m] + n];
    
    }

    void fill(T value)
    {
        for (int i = 0; i < dim_1; i++)
        {
            for (int j = 0; j < dim_2; j++)
            {
                for (int k = 0; k < dim_3; k++)
                {
                    for (int l = 0; l < dim_4; l++)
                    {
                        for (int m = 0; m < dim_5; m++)
                        {
                            for (int n = 0; n < dim_6; n++)
                            {
                                at(i, j, k, l, m, n) = value;
                            }
                        }
                    }
                }
            }
        }
    }

}; // end array6D

//=====================================================================================

struct CONFIG
{
        int*   iA;
        float* fA;
        bool*  bA;
        string* sA;

        size_t i_size;
        size_t f_size;
        size_t b_size;
        size_t s_size;

        CONFIG(size_t i_s, size_t f_s, size_t b_s, size_t s_s) : i_size(i_s), f_size(f_s), b_size(b_s), s_size(s_s)
        {
                iA = new int[i_s]();
                fA = new float[f_s]();
                bA = new bool[b_s]();
                sA = new string[s_s]();
        }
        ~CONFIG()
        {
                delete[] iA;
                delete[] fA;
                delete[] bA;
                delete[] sA;
        }

        int& i(size_t index)
        {
                return iA[index];
        }

        float& f(size_t index)
        {
                return fA[index];
        }

        bool& b(size_t index)
        {
                return bA[index];
        }

        string& s(size_t index)
        {
                return sA[index];
        }
};
