#include <forward_list>	 // singly linked list for caching false +ve results from bloom filter
#include <iostream>
#include <set>	  // used to check memberships while compressing graph
#include <stack>  // used for DFS

#include "bloom_filter.hpp"
using namespace std;

const bool PRINT_GRAPH = true;
const int TOTAL_NODES = 10000;															 // total nodes in the entire graph
const int CACHE_LEN_LIMIT_AS_PC_OF_TOTAL_NODES = 1;										 // cache wont have more than 1% of TOTAL_NODES
const int CACHE_LEN_LIMIT = (CACHE_LEN_LIMIT_AS_PC_OF_TOTAL_NODES / 100) * TOTAL_NODES;	 // hence the false +ve probability is also 1%

vector<pair<vector<int>, bloom_filter>> adjacencyList;	// used to represent the graph
forward_list<string> falsePositiveCache;				// caches the results that turned out to be false +ve

bloom_filter createBloomFilter(const int number) {
	bloom_parameters parameters;
	parameters.projected_element_count = number;  // max number of elements the bloom filter can contain
	parameters.false_positive_probability = (float)CACHE_LEN_LIMIT_AS_PC_OF_TOTAL_NODES / 100;
	parameters.compute_optimal_parameters();
	bloom_filter bf(parameters);
	return bf;
}
void GraphBuilder() {
	adjacencyList.resize(TOTAL_NODES + 1);	// to avoid Amortized O(1) insertions

	// Building the adjacency list for uncompressed graph
	for (int i = 2; i <= TOTAL_NODES; i++) {
		adjacencyList[i].second = bloom_filter();  // creating an empty bloom filter as the length of vectors in the adjacency list is unknown
		for (int j = 2; j < i; j++)
			if (i % j == 0) adjacencyList[i].first.push_back(j);
	}

	// setting up bloom filters from uncompressed graph
	for (int i = 2; i <= TOTAL_NODES; i++) {
		const vector<int> &factors = adjacencyList[i].first;
		adjacencyList[i].second = createBloomFilter(factors.size());
		for (const int &factor: factors) adjacencyList[i].second.insert(factor);
	}

	// Compressing the graph inplace
	for (int i = TOTAL_NODES; i >= 2; i--) {
		vector<int> &listToBeCompressed = adjacencyList[i].first;
		for (int j = listToBeCompressed.size() - 1; j >= 0; j--) {
			int largerNumber = listToBeCompressed[j];
			set<int> factorsOfTheLargerNumber(adjacencyList[largerNumber].first.cbegin(), adjacencyList[largerNumber].first.cend());

			for (int k = listToBeCompressed.size() - 1; k >= 0; k--)
				if (factorsOfTheLargerNumber.count(listToBeCompressed[k]))
					listToBeCompressed.erase(listToBeCompressed.cbegin() + k);	// Removes the common factors
		}
	}

	cout << "GRAPH HAS BEEN BUILT!" << endl;
	if (not PRINT_GRAPH) return;

	// Printing the compressed graph
	for (int i = 2; i <= TOTAL_NODES; i++) {
		cout << "Factors of " << i << " : ";
		for (const int &factor: adjacencyList[i].first) cout << factor << " ";
		cout << "\n";
	}
}

string generateKey(const int isThisNumber, const int aFactorOfThisNumber) { return to_string(isThisNumber) + ',' + to_string(aFactorOfThisNumber); }

void pruneCache() {
	const int cacheLen = distance(falsePositiveCache.cbegin(), falsePositiveCache.cend());
	if (cacheLen < CACHE_LEN_LIMIT) {
		auto justBeforeTail = next(falsePositiveCache.cbegin(), cacheLen - 1);
		falsePositiveCache.erase_after(justBeforeTail);	 // deletes the tail
	}
}
void cacheFalsePositiveResult(const int isThisNumber, const int aFactorOfThisNumber) {
	falsePositiveCache.push_front(generateKey(isThisNumber, aFactorOfThisNumber));
	pruneCache();
}
bool inCache(const int isThisNumber, const int aFactorOfThisNumber) {
	for (const string &falsePositive: falsePositiveCache)
		if (generateKey(isThisNumber, aFactorOfThisNumber) == falsePositive) {
			// if query in cache, move it to front so that response is faster next time
			// this way least queried items(one hit wonders) are implicitly moved towards tailed and eventually gets pruned
			falsePositiveCache.remove(falsePositive);
			falsePositiveCache.push_front(falsePositive);
			return true;
		}
	return false;
}

bool searchUsingDFS(const int isThisNumber, const int aFactorOfThisNumber) {
	stack<int> dfsStack;
	dfsStack.push(aFactorOfThisNumber);

	while (not dfsStack.empty()) {
		const int currentNode = dfsStack.top();
		if (currentNode == isThisNumber) return true;
		dfsStack.pop();
		for (const int &factorOfCurrentNode: adjacencyList[currentNode].first) dfsStack.push(factorOfCurrentNode);
	}

	return false;
}

bool searchUsingBloomFilter(const int isThisNumber, const int aFactorOfThisNumber) {
	bool result = adjacencyList[isThisNumber].second.contains(aFactorOfThisNumber);
	if (result == false) return false;	// if result==false, then result is definately false

	// otherwise it may be a false positive, so check the cache that maintains previous false positive results
	if (inCache(isThisNumber, aFactorOfThisNumber)) return false;

	result = searchUsingDFS(isThisNumber, aFactorOfThisNumber);
	if (result == false) cacheFalsePositiveResult(isThisNumber, aFactorOfThisNumber);
	return result;
}

#include <chrono>
void compareExecTime() {
	using namespace std::chrono;

	auto start = high_resolution_clock::now();
	searchUsingDFS(2, TOTAL_NODES - 2);
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	cout << "DFS took: " << duration.count() << " Microseconds" << endl;

	start = high_resolution_clock::now();
	searchUsingBloomFilter(2, TOTAL_NODES - 2);
	stop = high_resolution_clock::now();
	duration = duration_cast<microseconds>(stop - start);
	cout << "DFS + Bloom Filter with False+ve Caching took: " << duration.count() << " Microseconds" << endl;
}

int main() {
	ios_base::sync_with_stdio(false);  // improves io in c++

	cout << "Please wait while the graph of " << TOTAL_NODES << " nodes is being generated, this might take a while..." << endl;
	GraphBuilder();

	cout << "\n\n[1] Query using DFS" << endl;
	cout << "[2] Query using DFS+Caching+BloomFilter" << endl;
	cout << "[3] Compare Execution Time of [1] and [2]" << endl;
	cout << "[0] Exit" << endl;

	int choice;
	while (true) {
		cout << "\n-> ";
		cin >> choice;
		if (choice == 0) return 0;
		if (choice != 1 and choice != 2 and choice != 3) {
			cout << "Invalid Choice, Try Again!" << endl;
			continue;
		}
		if (choice == 3) {
			compareExecTime();
			continue;
		}

		cout << "Check if X is a factor of Y" << endl;
		int x, y;
		cout << "Enter X: ";
		cin >> x;
		cout << "Enter Y: ";
		cin >> y;
		if (x > TOTAL_NODES or y > TOTAL_NODES) {
			cout << "Input Out Of Range, Try Again!" << endl;
			continue;
		}

		switch (choice) {
			case 1:
				cout << x << " is " << (searchUsingDFS(x, y) ? "" : "not a ") << "factor of " << y << endl;
				break;
			case 2:
				cout << x << " is " << (searchUsingBloomFilter(x, y) ? "" : "not a ") << "factor of " << y << endl;
		}
	}

	return -1;
}