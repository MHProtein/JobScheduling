#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>
#include <unordered_map>

#include "Scheduler.h"
#include "Job.h"
#include "tsl/bhopscotch_map.h"


std::vector<JobScheduling::Job> generateRandomJobs(int numJobs, int maxWeight, float dependencyChance) {
    std::vector<JobScheduling::Job> jobs;

    std::srand(time(0));

    for (int i = 0; i < numJobs; ++i) {
	    JobScheduling::Job job;
        job.id = i;
        job.weight = std::rand() % maxWeight + 1; 

        for (int j = 0; j < i; ++j) {
            if ((std::rand() / (float)RAND_MAX) < dependencyChance) {
                job.dependencies.push_back(j);
            }
        }

        jobs.push_back(job);
    }
    jobs[0].dependencies.clear();
    return jobs;
}

void printJobs(const std::vector<JobScheduling::Job>& jobs) {
    std::cout << "std::vector<JobScheduling::Job> jobs = {\n";
    for (const auto& job : jobs) {
        std::cout << "    { " << job.id << ", " << job.weight << ", {";
        for (size_t i = 0; i < job.dependencies.size(); ++i) {
            std::cout << job.dependencies[i];
            if (i < job.dependencies.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "} },\n";
    }
    std::cout << "};" << std::endl;
}

int main(int argc, char* argv[])
{
    JobScheduling::Scheduler scheduler;
    std::vector<JobScheduling::Job> jobs = {
{ 0, 3, {} },
    { 1, 2, {} },
    { 2, 3, {0, 1} },
    { 3, 1, {1} },
    { 4, 1, {2, 3} },
    { 5, 3, {0} },
    { 6, 3, {0, 4} },
    { 7, 1, {2, 3, 4} },
    { 8, 3, {1} },
    { 9, 1, {2, 6} },
    { 10, 3, {2, 4, 8} },
    { 11, 2, {0, 2, 3, 4, 6, 7} },
    { 12, 2, {10} },
    { 13, 2, {0, 1, 5, 8} },
    { 14, 3, {0, 1, 5, 8} },
    };

    std::chrono::time_point<std::chrono::steady_clock> dpStart;
    std::chrono::time_point<std::chrono::steady_clock> dpEnd;
    std::chrono::duration<float, std::milli> dpDuration;


    std::chrono::time_point<std::chrono::steady_clock> gaStart;
    std::chrono::time_point<std::chrono::steady_clock> gaEnd;
    std::chrono::duration<float, std::milli> gaDuration;

    

    for(int i = 0; i != 100; ++i)
    {
        auto x = generateRandomJobs(10, 3, 0.3f);
        scheduler.Reset(2, 150, x);
        printJobs(x);


        dpStart = std::chrono::high_resolution_clock::now();
        std::cout << scheduler.DynamicProgramming() << " ";
        dpEnd = std::chrono::high_resolution_clock::now();
        dpDuration = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(dpEnd - dpStart);
        std::cout << dpDuration.count() / 1000 << std::endl;

        gaStart = std::chrono::high_resolution_clock::now();
        std::cout << scheduler.ApproximationScheme(0.3f, 1000, 5000, 100) << " ";
    	//std::cout << scheduler.Genetic( 1000, 100) << " ";
        gaEnd = std::chrono::high_resolution_clock::now();
        gaDuration = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(gaEnd - gaStart);
        std::cout << gaDuration.count() / 1000 << std::endl;
        std::cout << std::endl;
    }
}