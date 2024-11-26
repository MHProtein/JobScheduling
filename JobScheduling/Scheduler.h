#pragma once
#include <unordered_set>
#include <vector>


namespace JobScheduling
{
	struct Job;

	class Scheduler
	{
		struct Chromosome
		{
			std::pair<int, int> fitness;
			std::vector<int> schedule;
			std::vector<int> machines;

			bool operator<(const Chromosome& other) const
			{
				if (fitness.second < other.fitness.second)
					return true;
				if (fitness.second == other.fitness.second)
					return fitness.first < other.fitness.first;
				return false;
			}

		};

		struct DPState
		{
			std::vector<int> machines;
			std::vector<int> schedule;
			int makespan;
			bool operator<(const DPState& other) const {
				return makespan < other.makespan;
			}
			bool operator==(const DPState& other) const {
				return makespan == other.makespan && machines == other.machines && schedule == other.schedule;
			}

			struct Hash {
				size_t operator()(const DPState& state) const {
					size_t hash = 0;

					hash ^= std::hash<int>{}(state.makespan) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

					for (int machine : state.machines) {
						hash ^= std::hash<int>{}(machine)+0x9e3779b9 + (hash << 6) + (hash >> 2);
					}

					for (int job : state.schedule) {
						hash ^= std::hash<int>{}(job)+0x9e3779b9 + (hash << 6) + (hash >> 2);
					}

					return hash;
				}
			};
		};

	public:
		Scheduler();

		Scheduler(int machines, int deadline, const std::vector<Job>& jobs);
		Scheduler(int machines, int deadline, std::vector<Job>&& jobs);
		void Reset(int machines, int deadline, const std::vector<Job>& jobs);
		void Reset(int machines, int deadline, std::vector<Job>&& jobs);

		int Genetic(int generations, int maxGeneration, int populationSize);
		int DynamicProgramming();
		int ApproximationScheme(float epsilon, int generations, int maxGeneration, int populationSize);


		//Genetic
		bool CanSchedule(const Job& job, const std::vector<int>& completed) const;
		std::pair<int, int> FitnessFunction(const Chromosome& chromosome) const;
		std::vector<Chromosome> GenerateInitialPopulation(int populationSize, int from, int to, const std::vector<int>& initSchedule,
			const std::vector<int>& initMachines) const;
		Chromosome Crossover(const Chromosome& parent1, const Chromosome& parent2, int from, int to);
		void Mutate(Chromosome& chromosome, int from, int to);
		std::tuple<int, std::vector<int>> GetMakespan(const std::vector<int>& schedule, const std::vector<int>& machines) const;

		Chromosome DoGenetic(int generations, int maxGeneration, int populationSize, int from, int to, const std::vector<int>& initSchedule,
			const std::vector<int>& initMachines);

		//DP
		DPState DoDPHash(int n, int begin, int end);

		int machinesCount;
		int deadline;
		std::vector<Job> jobs;
	};
}



