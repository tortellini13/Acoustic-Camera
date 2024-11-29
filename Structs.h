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

struct cfloat4D
{
    cfloat*  data; // pointer to data array
    size_t dim_1; // x dimension
    size_t dim_2; // y dimension
    size_t dim_3; // z dimension
    size_t dim_4; // w dimension

    cfloat4D(size_t d1, size_t d2, size_t d3, size_t d4) : dim_1(d1), dim_2(d2), dim_3(d3), dim_4(d4) 
    {
        data = new cfloat[d1 * d2 * d3 * d4](); // Initialize array
    }

    ~cfloat4D() 
    {
        delete[] data;
    }

    // Compute flat index and access element
    cfloat& at(size_t i, size_t j, size_t k, size_t l) 
    {
        return data[i * dim_2 * dim_3 * dim_4 + j * dim_3 * dim_4 + k * dim_4 + l];
    }

    // For read-only access
    const cfloat& at(size_t i, size_t j, size_t k, size_t l) const 
    {
        return data[i * dim_2 * dim_3 * dim_4 + j * dim_3 * dim_4 + k * dim_4 + l];
    }
}; // end float4D