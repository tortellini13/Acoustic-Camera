#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <string>

#include "PARAMS.h"

using namespace std;

void launchProgram(pid_t& pid, const char* execCommand)
{
    // Stores the command to launch the .exe
    string programName = execCommand;
    programName.erase(0,2);
    
    // Fork a child process to open the program later
    pid = fork(); // Returns -1 if error, 0+ if success

    // Error handling
    if (pid < 0)
    {
        cerr << "Failed to execute " << programName << endl;
        exit(1);
    }

    // Launch the program
    else if (pid == 0)
    {
        cout << pid;
        execl(execCommand, execCommand, NULL);
        cerr << "Failed to execute " << programName << endl;
        exit(1);
    }
} // end launchProgram

int main()
{
    pid_t pid1, pid2 = 0; // Stands for process identifier. System needs it to order child processes

    const char* Audio = "./Audio";
    const char* Video = "./Video";

    launchProgram(pid1, Audio);
    sleep(1);
    launchProgram(pid2, Video);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    cout << "\nPrograms are done\n";

    return 0;

}