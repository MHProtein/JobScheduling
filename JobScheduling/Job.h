#pragma once
#include <vector>

namespace JobScheduling
{
	struct Job
	{
		int id;
		int weight;
		std::vector<int> dependencies;

		Job();
		Job(int id, int weight, const std::vector<int>& dependencies);
	};

	bool operator<(const Job& lhs, const Job& rhs);
	bool operator==(const Job& lhs, const Job& rhs);
	bool operator>(const Job& lhs, const Job& rhs);
}

