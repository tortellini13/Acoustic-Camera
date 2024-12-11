#pragma once

#include <iostream>
#include <complex>

using namespace std;

//=====================================================================================

typedef complex<float> cfloat;

struct float2D
{
    float*  data; // pointer to data array
    size_t dim_1; // x dimension
    size_t dim_2; // y dimension

    float2D(size_t d1, size_t d2) : dim_1(d1), dim_2(d2)
    {
        data = new float[d1 * d2](); // Initialize array
    }

    ~float2D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    float& at(size_t i, size_t j) 
    {
        return data[i * dim_2 + j];
    }

    // For read-only access
    const float& at(size_t i, size_t j) const 
    {
        return data[i * dim_2 + j];
    }
}; // end float2D

//=====================================================================================

struct cfloat2D
{
    cfloat*  data; // pointer to data array
    size_t dim_1; // x dimension
    size_t dim_2; // y dimension

    cfloat2D(size_t d1, size_t d2) : dim_1(d1), dim_2(d2)
    {
        data = new cfloat[d1 * d2](); // Initialize array
    }

    ~cfloat2D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    cfloat& at(size_t i, size_t j) 
    {
        return data[i * dim_2 + j];
    }

    // For read-only access
    const cfloat& at(size_t i, size_t j) const 
    {
        return data[i * dim_2 + j];
    }
}; // end cfloat2D

//=====================================================================================

struct int2D
{
    int*  data; // pointer to data array
    size_t dim_1; // x dimension
    size_t dim_2; // y dimension

    int2D(size_t d1, size_t d2) : dim_1(d1), dim_2(d2)
    {
        data = new int[d1 * d2](); // Initialize array
    }

    ~int2D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    int& at(size_t i, size_t j) 
    {
        return data[i * dim_2 + j];
    }

    // For read-only access
    const int& at(size_t i, size_t j) const 
    {
        return data[i * dim_2 + j];
    }
}; // end int2D

//=====================================================================================

struct float3D
{
    float*  data; // pointer to data array
    size_t dim_1; // x dimension
    size_t dim_2; // y dimension
    size_t dim_3; // z dimension

    float3D(size_t d1, size_t d2, size_t d3) : dim_1(d1), dim_2(d2), dim_3(d3) 
    {
        data = new float[d1 * d2 * d3](); // Initialize array
    }

    ~float3D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    float& at(size_t i, size_t j, size_t k) 
    {
        return data[i * dim_2 * dim_3 + j * dim_3 + k];
    }

    // For read-only access
    const float& at(size_t i, size_t j, size_t k) const 
    {
        return data[i * dim_2 * dim_3 + j * dim_3 + k];
    }
}; // end float3D

//=====================================================================================

struct cfloat3D
{
    cfloat*  data; // pointer to data array
    size_t dim_1; // x dimension
    size_t dim_2; // y dimension
    size_t dim_3; // z dimension

    cfloat3D(size_t d1, size_t d2, size_t d3) : dim_1(d1), dim_2(d2), dim_3(d3) 
    {
        data = new cfloat[d1 * d2 * d3](); // Initialize array
    }

    ~cfloat3D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    cfloat& at(size_t i, size_t j, size_t k) 
    {
        return data[i * dim_2 * dim_3 + j * dim_3 + k];
    }

    // For read-only access
    const cfloat& at(size_t i, size_t j, size_t k) const 
    {
        return data[i * dim_2 * dim_3 + j * dim_3 + k];
    }
}; // end cfloat3D

//=====================================================================================

struct cfloat5D
{
    cfloat* data; // pointer to data array
    size_t dim_1; // x1 dimension
    size_t dim_2; // x2 dimension
    size_t dim_3; // x3 dimension
    size_t dim_4; // x4 dimension
    size_t dim_5; // x5 dimension

    cfloat5D(size_t d1, size_t d2, size_t d3, size_t d4, size_t d5) : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4), dim_5(d5)
    {
        data = new cfloat[d1 * d2 * d3 * d4 * d5](); // Initialize array
    }

    ~cfloat5D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    cfloat& at(size_t i, size_t j, size_t k, size_t l, size_t m) 
    {
        return data[i * dim_2 * dim_3 * dim_4 * dim_5 + j * dim_3 * dim_4 * dim_5 + k * dim_4 * dim_5 + l * dim_5 + m];
    }

    // For read-only access
    const cfloat& at(size_t i, size_t j, size_t k, size_t l, size_t m) const 
    {
        return data[i * dim_2 * dim_3 * dim_4 * dim_5 + j * dim_3 * dim_4 * dim_5 + k * dim_4 * dim_5 + l * dim_5 + m];
    }
}; // end float5D