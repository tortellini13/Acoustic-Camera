#pragma once

#include <iostream>
#include <chrono>

class timer
{
public:
    timer(const char* timer_name);

    ~timer();

    void start();

    void end();

    void print(bool is_ms = true);

    void print_avg(int avg_count, bool is_ms = true);

    double time(bool is_ms = true);

    double time_avg(int num_samples, bool is_ms = true);

    int getCurrentAvgCount();

private:
    const char* timer_name;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    std::chrono::duration<double> elapsed_time;

    double avg_time = 0;
    int avg_counter = 0;
};

//=====================================================================================

timer::timer(const char*timer_name):
    timer_name(timer_name)
    {}

timer::~timer()
{

} // end ~timer

//=====================================================================================

void timer::start()
{
    start_time = std::chrono::high_resolution_clock::now();
} // end start

void timer::end()
{
    end_time = std::chrono::high_resolution_clock::now();
    elapsed_time = end_time - start_time;
} // end end

//=====================================================================================

void timer::print(bool is_ms)
{
    if (!is_ms) {std::cout << timer_name << ": " << elapsed_time.count() << " seconds.\n";}
    else        {std::cout << timer_name << ": " << elapsed_time.count() * 1000 << " ms.\n";}
    
} // end print

void timer::print_avg(int num_samples, bool is_ms)
{
    // Sum a number of samples of time
    if (avg_counter < num_samples)
    {
        avg_time += elapsed_time.count();
        avg_counter++;
    }
    // Print the average time
    else
    {
        avg_time /= num_samples;

        if (!is_ms) {std::cout << timer_name << ": " << avg_time << " seconds.\n";}
        else {std::cout << timer_name << ": " << avg_time * 1000 << " ms.\n";}

        // Reset avg
        avg_time = 0;
        avg_counter = 0;
    }
} // end print_avg

//=====================================================================================

double timer::time(bool is_ms)
{
    if (!is_ms) {return elapsed_time.count();}
    else        {return elapsed_time.count() * 1000;}
} // end time

// Returns -1 if still averaging
double timer::time_avg(int num_samples, bool is_ms)
{
    double time;
    // Sum a number of samples of time
    if (avg_counter < num_samples)
    {
        avg_time += elapsed_time.count();
        avg_counter++;

        return -1;
    }

    // Return average time
    else
    {
        avg_time /= num_samples;

        if (!is_ms) {return avg_time;}
        else        {return avg_time * 1000;}

        // Reset avg
        avg_time = 0;
        avg_counter = 0;
    }
} // end time_avg

//=====================================================================================

int timer::getCurrentAvgCount()
{
    return avg_counter;
} // end getCurrentAvgCount