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

    // Copy Constructor
    array3D(const array3D& other) : dim_1(other.dim_1), dim_2(other.dim_2), dim_3(other.dim_3)
    {
        data = new T[dim_1 * dim_2 * dim_3];
        for (size_t i = 0; i < dim_1 * dim_2 * dim_3; ++i)
        {
            data[i] = other.data[i];
        }
    }

    // Copy Assignment Operator
    array3D& operator=(const array3D& other)
    {
        if (this == &other)  // Prevent self-assignment
            return *this;

        // Free existing memory
        delete[] data;

        // Copy the dimensions
        dim_1 = other.dim_1;
        dim_2 = other.dim_2;
        dim_3 = other.dim_3;

        // Allocate new memory and copy data
        data = new T[dim_1 * dim_2 * dim_3];
        for (size_t i = 0; i < dim_1 * dim_2 * dim_3; ++i)
        {
            data[i] = other.data[i];
        }

        return *this;
    }

    // Move Constructor
    array3D(array3D&& other) noexcept
        : data(other.data), dim_1(other.dim_1), dim_2(other.dim_2), dim_3(other.dim_3)
    {
        // Leave other in a valid state
        other.data = nullptr;
        other.dim_1 = 0;
        other.dim_2 = 0;
        other.dim_3 = 0;
    }

    // Move Assignment Operator
    array3D& operator=(array3D&& other) noexcept
    {
        if (this == &other)  // Prevent self-assignment
            return *this;

        // Free existing memory
        delete[] data;

        // Transfer ownership of data
        data = other.data;
        dim_1 = other.dim_1;
        dim_2 = other.dim_2;
        dim_3 = other.dim_3;

        // Leave other in a valid state
        other.data = nullptr;
        other.dim_1 = 0;
        other.dim_2 = 0;
        other.dim_3 = 0;

        return *this;
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

    // Print array layer
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

    // Constructor
    array5D(size_t d1, size_t d2, size_t d3, size_t d4, size_t d5) 
        : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4), dim_5(d5)
    {
        data = new T[d1 * d2 * d3 * d4 * d5](); // Allocate and zero-initialize the array
    }

    // Destructor
    ~array5D()
    {
        delete[] data;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k, size_t l, size_t m)
    {
        return data[i * dim_2 * dim_3 * dim_4 * dim_5 + j * dim_3 * dim_4 * dim_5 + k * dim_4 * dim_5 + l * dim_5 + m];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k, size_t l, size_t m) const
    {
        return data[i * dim_2 * dim_3 * dim_4 * dim_5 + j * dim_3 * dim_4 * dim_5 + k * dim_4 * dim_5 + l * dim_5 + m];
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

    // Constructor
    array6D(size_t d1, size_t d2, size_t d3, size_t d4, size_t d5, size_t d6) 
        : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4), dim_5(d5), dim_6(d6)
    {
        data = new T[d1 * d2 * d3 * d4 * d5 * d6](); // Allocate and zero-initialize the array
    }

    // Destructor
    ~array6D()
    {
        delete[] data;
    }

    // Access element (read/write)
    T& at(size_t i, size_t j, size_t k, size_t l, size_t m, size_t n)
    {
        return data[i * dim_2 * dim_3 * dim_4 * dim_5 * dim_6 + j * dim_3 * dim_4 * dim_5 * dim_6 + k * dim_4 * dim_5 * dim_6 + l * dim_5 * dim_6 + m * dim_6 + n];
    }

    // Access element (read-only)
    const T& at(size_t i, size_t j, size_t k, size_t l, size_t m, size_t n) const
    {
        return data[i * dim_2 * dim_3 * dim_4 * dim_5 * dim_6 + j * dim_3 * dim_4 * dim_5 * dim_6 + k * dim_4 * dim_5 * dim_6 + l * dim_5 * dim_6 + m * dim_6 + n];
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
