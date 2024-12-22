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

    void print();

    void print_ms();

    double time() const;

    double time_ms() const;

private:
    const char* timer_name;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    std::chrono::duration<double> elapsed_time;

};

timer::timer(const char*timer_name):
    timer_name(timer_name)
    {}

timer::~timer()
{

} // end ~timer

void timer::start()
{
    start_time = std::chrono::high_resolution_clock::now();
} // end start

void timer::end()
{
    end_time = std::chrono::high_resolution_clock::now();
    elapsed_time = end_time - start_time;
} // end end

void timer::print()
{
    std::cout << timer_name << ": " << elapsed_time.count() << " seconds.\n";
} // end print

void timer::print_ms()
{
    std::cout << timer_name << ": " << elapsed_time.count() * 1000 << " ms.\n";
} // end print_ms

double timer::time() const
{
    return elapsed_time.count();
} // end time

double timer::time_ms() const
{
    return elapsed_time.count() * 1000;
} // end time_ms