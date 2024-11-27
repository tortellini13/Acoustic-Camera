#ifndef TIME_H
#define TIMER_H

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

    double time() const;

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

double timer::time() const
{
    return elapsed_time.count();
} // end time

#endif