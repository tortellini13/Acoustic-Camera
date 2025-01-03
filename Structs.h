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

    // Constructor
    array2D(size_t d1, size_t d2) : dim_1(d1), dim_2(d2)
    {
        data = new T[d1 * d2](); // Allocate and zero-initialize the array
    }

    // Destructor
    ~array2D()
    {
        delete[] data;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j)
    {
        return data[i * dim_2 + j];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j) const
    {
        return data[i * dim_2 + j];
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

    // Constructor
    array3D(size_t d1, size_t d2, size_t d3) : dim_1(d1), dim_2(d2), dim_3(d3)
    {
        data = new T[d1 * d2 * d3](); // Allocate and zero-initialize the array
    }

    // Destructor
    ~array3D()
    {
        delete[] data;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k)
    {
        return data[i * dim_2 * dim_3 + j * dim_3 + k];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k) const
    {
        return data[i * dim_2 * dim_3 + j * dim_3 + k];
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

    // Constructor
    array4D(size_t d1, size_t d2, size_t d3, size_t d4) 
        : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4)
    {
        data = new T[d1 * d2 * d3 * d4](); // Allocate and zero-initialize the array
    }

    // Destructor
    ~array4D()
    {
        delete[] data;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k, size_t l)
    {
        return data[i * dim_2 * dim_3 * dim_4 + j * dim_3 * dim_4 + k * dim_4 + l];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k, size_t l) const
    {
        return data[i * dim_2 * dim_3 * dim_4 + j * dim_3 * dim_4 + k * dim_4 + l];
    }
}; // end array4D
