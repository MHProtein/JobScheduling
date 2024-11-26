#include "Scheduler.h"

#include <bitset>
#include <functional>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>
#include <flat_hash_map.hpp>

#include "Job.h"

JobScheduling::Scheduler::Scheduler()
{
}

JobScheduling::Scheduler::Scheduler(int machines, int deadline, const std::vector<Job>& jobs)
	: machinesCount(machines), deadline(deadline), jobs(jobs)
{
}

JobScheduling::Scheduler::Scheduler(int machines, int deadline, std::vector<Job>&& jobs)
	: machinesCount(machines), deadline(deadline), jobs(std::move(jobs))
{
}

void JobScheduling::Scheduler::Reset(int machines, int deadline, const std::vector<Job>& jobs)
{
	this->machinesCount = machines;
	this->deadline = deadline;
	this->jobs = jobs;
}

void JobScheduling::Scheduler::Reset(int machines, int deadline, std::vector<Job>&& jobs)
{
	this->machinesCount = machines;
	this->deadline = deadline;
	this->jobs = std::move(jobs);
}

std::tuple<int, std::vector<int>> JobScheduling::Scheduler::GetMakespan(const std::vector<int>& schedule, const std::vector<int>& machines) const
{
	std::vector<int> machineTimes(machinesCount, 0);
	std::vector<int> jobCompletionTime(jobs.size(), 0);

	for (int i = 0; i < schedule.size(); ++i)
	{
		int jobId = schedule[i];
		int processorId = machines[jobId];
		int earliestStart = 0;
		for (int dep : jobs[jobId].dependencies) {
			earliestStart = std::max(earliestStart, jobCompletionTime[dep]);
		}

		int startTime = std::max(earliestStart, machineTimes[processorId]);
		int finishTime = startTime + jobs[jobId].weight;

		machineTimes[processorId] = finishTime;
		jobCompletionTime[jobId] = finishTime;
	}

	return { *std::max_element(jobCompletionTime.begin(), jobCompletionTime.end()), machineTimes };
}

bool JobScheduling::Scheduler::CanSchedule(const Job& job, const std::vector<int>& completed) const
{
	for (int dep : job.dependencies)
	{
		if (std::ranges::find(completed.begin(), completed.end(), dep) == completed.end()) 
			return false;
	}
	return true;
}

std::pair<int, int> JobScheduling::Scheduler::FitnessFunction(const Chromosome& chromosome) const
{
	std::vector<int> completed;

	int penalty = 0;
	for (int i = 0; i < chromosome.schedule.size(); ++i)
	{
		int jobID = chromosome.schedule[i];
		const Job& job = jobs[jobID];

		if(!CanSchedule(job, completed))
			penalty += 1;

		completed.push_back(jobID);
	}

	int makespan = std::get<int>(GetMakespan(chromosome.schedule, chromosome.machines));
	return { makespan, penalty };
}

std::vector<JobScheduling::Scheduler::Chromosome> JobScheduling::Scheduler::GenerateInitialPopulation(int populationSize, int from, int to, const std::vector<int>& initSchedule,
	const std::vector<int>& initMachines) const
{
	std::vector<Chromosome> population;

	static std::random_device device;
	static std::mt19937 rng(device());

	for (int i = 0; i < populationSize;)
	{
		Chromosome chromosome;
		chromosome.schedule.resize(jobs.size());
		chromosome.machines.resize(jobs.size());

		if(!initSchedule.empty())
		{
			for (int i = 0; i < from; ++i)
			{
				chromosome.schedule[i] = initSchedule[i];
				chromosome.machines[i] = initMachines[i];
			}
		}

		std::iota(chromosome.schedule.begin() + from, chromosome.schedule.begin() + to, from);
		std::shuffle(chromosome.schedule.begin() + from, chromosome.schedule.begin() + to, rng);

		for (int j = from; j < to; ++j)
		{
			chromosome.machines[j] = rng() % machinesCount;
		}
		chromosome.fitness = FitnessFunction(chromosome);
		if(chromosome.fitness.second > 5)
			continue;
		++i;
		population.push_back(chromosome);
	}
	return population;
}

JobScheduling::Scheduler::Chromosome JobScheduling::Scheduler::Crossover(const Chromosome& parent1, const Chromosome& parent2, int from, int to)
{
	static std::random_device device;
	static std::mt19937 rng(device());
	static std::uniform_int_distribution<int> distribution(from, parent1.schedule.size() - 1);

	int crossOverPoint = distribution(rng);
	Chromosome child;

	if(from > 0)
	{
		child.machines.resize(from);
		child.schedule.resize(from);
		for (int i = 0; i != from; ++i)
		{
			child.machines[i] = parent1.machines[i];
			child.schedule[i] = parent1.schedule[i];
		}
	}

	child.schedule.insert(child.schedule.end(), parent1.schedule.begin() + from, parent1.schedule.begin() + crossOverPoint);

	for (const auto & job : parent2.schedule)
	{
		if (std::ranges::find(child.schedule.begin(), child.schedule.end(), job) == child.schedule.end())
			child.schedule.push_back(job);
	}
	child.machines = parent1.machines;
	child.fitness = std::make_pair(std::numeric_limits<int>::max(), 0);

	return child;
}

void JobScheduling::Scheduler::Mutate(Chromosome& chromosome, int from, int to)
{
	static std::random_device device;
	static std::mt19937 rng(device());
	static std::uniform_int_distribution<int> distribution(from, to - 1);

	int index1 = distribution(rng);
	int index2 = distribution(rng);

	std::swap(chromosome.schedule[index1], chromosome.schedule[index2]);

	chromosome.machines[index1] = rng() % machinesCount;
}

JobScheduling::Scheduler::Chromosome JobScheduling::Scheduler::DoGenetic(int generations, int maxGeneration, int populationSize, int from, int to, const std::vector<int>& initSchedule,
	const std::vector<int>& initMachines)
{
	auto population = GenerateInitialPopulation(populationSize, from, to, initSchedule, initMachines);

	bool shouldExit = false;

	int gen = 0;
	while (true)
	{
		if(gen >= generations)
		{
			shouldExit = true;
		}

		for (auto& chromosome : population)
			chromosome.fitness = FitnessFunction(chromosome);

		std::sort(population.begin(), population.end());

		if(shouldExit)
		{
			if(population[0].fitness.second == 0)
				break;
			if(gen >= maxGeneration)
				break;
		}

		std::vector<Chromosome> newPopulation(population.begin(), population.begin() + populationSize / 10);

		static std::random_device device;
		static std::mt19937 rng(device());
		static std::uniform_int_distribution<int> distribution(0, populationSize / 2 - 1);

		while (newPopulation.size() < populationSize)
		{
			int parent1 = distribution(rng);
			int parent2 = distribution(rng);

			Chromosome child = Crossover(population[parent1], population[parent2], from, to);

			if (distribution(rng) % 4 == 0)
				Mutate(child, from, to);
			child.fitness = FitnessFunction(child);
			newPopulation.push_back(child);
		}
		population = newPopulation;
		gen++;
	}

	auto result = *std::ranges::min_element(population.begin(), population.end(), [](const Chromosome& a, const Chromosome& b) {
		return a < b;
		});
	return result;
}


int JobScheduling::Scheduler::Genetic(int generations, int maxGeneration, int populationSize)
{
	auto chrom = DoGenetic(generations, maxGeneration, populationSize, 0, jobs.size(), {}, {});
	return std::get<int>(GetMakespan(chrom.schedule, chrom.machines));
}

int JobScheduling::Scheduler::DynamicProgramming()
{
	DPState finalState = DoDPHash(jobs.size(), 0, jobs.size());

	return std::get<int>(GetMakespan(finalState.schedule, finalState.machines));
}

int JobScheduling::Scheduler::ApproximationScheme(float epsilon, int generations, int maxGeneration, int populationSize)
{
	int m = epsilon * jobs.size();

	auto state = DoDPHash(m, 0, m);
	auto chrom = DoGenetic(generations, maxGeneration, populationSize, m, jobs.size(), state.schedule, state.machines);

	std::vector<int> schedule;
	schedule.insert(schedule.end(), state.schedule.begin(), state.schedule.begin() + m);
	schedule.insert(schedule.end(), chrom.schedule.begin() + m, chrom.schedule.end());

	std::vector<int> machines;
	machines.insert(machines.end(), state.machines.begin(), state.machines.begin() + m);
	machines.insert(machines.end(), chrom.machines.begin() + m, chrom.machines.end());

	return std::get<int>(GetMakespan(schedule, machines));
}


JobScheduling::Scheduler::DPState JobScheduling::Scheduler::DoDPHash(int n, int begin, int end)
{
	uint64_t fullMask = (1 << n) - 1;
	ska::flat_hash_map<uint64_t, tsl::hopscotch_set<DPState, DPState::Hash>> dp;
	
	dp.insert(std::make_pair(0, tsl::hopscotch_set<DPState, DPState::Hash>()));
	DPState initState = { std::vector(jobs.size(), -1),
		std::vector<int>(), 0 };
	dp[0].insert(initState);
	for (int mask = 0; mask != fullMask;)
	{
		int newMask = mask;
		for (auto & state : dp[mask])
		{
			if(state.makespan == std::numeric_limits<int>::max()) continue;

			for (int j = 0; j < n; ++j)
			{
				if (!(mask & (1 << j)) && CanSchedule(jobs[j + begin], state.schedule))
				{
					newMask = mask | (1 << j);
					if(dp.find(newMask) == dp.end())
						dp.insert(std::make_pair(newMask, tsl::hopscotch_set<DPState, DPState::Hash>()));

					DPState temp;
					temp.schedule = state.schedule;
					temp.schedule.push_back(j + begin);
					std::vector<int> machines;
					for (int p = 0; p < machinesCount; ++p)
					{
						machines = state.machines;
						machines[j + begin] = p;
						auto [makespan, machineTimes] = GetMakespan(temp.schedule, machines);
						temp.makespan = makespan;
						temp.machines = machines;
						dp[newMask].insert(temp);
					}
				}
			}
		}
		mask = newMask;
	}
	return *std::ranges::min_element(dp[fullMask].begin(), dp[fullMask].end(), [](const DPState& lhs, const DPState& rhs) {return lhs.makespan < rhs.makespan; });
}