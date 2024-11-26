#include "Job.h"

JobScheduling::Job::Job() : id(-1), weight(-1)
{
}

JobScheduling::Job::Job(int id, int weight, const std::vector<int>& dependencies)
	: id(id), weight(weight), dependencies(dependencies)
{
}

