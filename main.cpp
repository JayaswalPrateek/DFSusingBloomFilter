// https://hur.st/bloomfilter/?n=100&p=&m=400&k=4
#include <forward_list>
#include <iostream>
#include <set>
#include <stack>
#include <vector>

#include "bloom/bloom_filter.hpp"
using namespace std;

const int TOTAL_NODES = 10000;
const int CACHE_SIZE_LIMIT_AS_PC_OF_TOTAL_NODES = 1;  // cache wont have nodes more than 1% if TOTAL_NODES
const int CACHE_SIZE_LIMIT = (CACHE_SIZE_LIMIT_AS_PC_OF_TOTAL_NODES / 100) * TOTAL_NODES;

vector<pair<vector<int>, bloom_filter>> adjacencyList;
forward_list<string> falsePositiveCache;

bloom_filter createBloomFilter(const int number) {
	bloom_parameters parameters;
	parameters.projected_element_count = number;
	parameters.false_positive_probability = (float)CACHE_SIZE_LIMIT_AS_PC_OF_TOTAL_NODES / 100;	 // 1 in 100
	parameters.compute_optimal_parameters();
	bloom_filter bf(parameters);
	return bf;
}
void GraphBuilder() {
	adjacencyList.resize(TOTAL_NODES + 1);

	// Building the Adj. List for uncompressed graph
	for (int i = 2; i <= TOTAL_NODES; i++) {
		adjacencyList[i].second = bloom_filter();
		for (int j = 2; j < i; j++)
			if (i % j == 0) {
				adjacencyList[i].first.push_back(j);
			}
	}

	// filling bloom filters using uncompressed graph
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
			set<int> factorsOfLargerElement(adjacencyList[largerNumber].first.cbegin(), adjacencyList[largerNumber].first.cend());

			for (int k = listToBeCompressed.size() - 1; k >= 0; k--)
				if (factorsOfLargerElement.count(listToBeCompressed[k]))
					listToBeCompressed.erase(listToBeCompressed.cbegin() + k);	// Removed the common factors
		}
	}

	// Printing the compressed graph
	for (int i = 2; i <= TOTAL_NODES; i++) {
		cout << "Factors of " << i << " : ";
		for (const int &factor: adjacencyList[i].first) cout << factor << " ";
		cout << endl;
	}
	cout << "\n\nGraph & Bloom Filter built successfully!" << endl;
}

string generateKey(const int isThisNumber, const int aFactorOfThisNumber) { return to_string(isThisNumber) + ',' + to_string(aFactorOfThisNumber); }

void pruneCache() {
	if (const int cacheSize = distance(falsePositiveCache.cbegin(), falsePositiveCache.cend()); cacheSize < CACHE_SIZE_LIMIT) {
		auto justBeforeTail = next(falsePositiveCache.cbegin(), cacheSize - 1);
		falsePositiveCache.erase_after(justBeforeTail);	 // deletes the tail
	}
}
void cacheFalsePositive(const int isThisNumber, const int aFactorOfThisNumber) {
	const string key = generateKey(isThisNumber, aFactorOfThisNumber);
	falsePositiveCache.push_front(key);
	pruneCache();
}
bool inCache(const int isThisNumber, const int aFactorOfThisNumber) {
	const string key = generateKey(isThisNumber, aFactorOfThisNumber);
	for (const string &falsePositive: falsePositiveCache)
		if (key == falsePositive) return true;
	return false;
}

bool searchUsingDFS(const int isThisNumber, const int aFactorOfThisNumber) {
	stack<int> dfsStack;
	dfsStack.push(isThisNumber);
	vector<bool> visited(TOTAL_NODES + 1, false);

	while (not dfsStack.empty()) {
		int currentNode = dfsStack.top();
		if (currentNode == aFactorOfThisNumber) return true;
		dfsStack.pop();
		visited[currentNode] = true;
		for (const int &factorOfCurrentNode: adjacencyList[currentNode].first)
			if (not visited[factorOfCurrentNode])
				dfsStack.push(factorOfCurrentNode);
	}

	return false;
}

bool searchUsingBloomFilter(const int isThisNumber, const int aFactorOfThisNumber) {
	bool result = adjacencyList[isThisNumber].second.contains(aFactorOfThisNumber);
	if (result == false) return false;	// if result==false, then result is definately false

	// otherwise it may be a false positive, so check the cache that maintains false positives
	if (inCache(isThisNumber, aFactorOfThisNumber)) return false;

	result = searchUsingDFS(isThisNumber, aFactorOfThisNumber);
	if (result == false) cacheFalsePositive(isThisNumber, aFactorOfThisNumber);
	return result;
}

int main() {
	cout << "Please wait while the graph of " << TOTAL_NODES << " nodes is being generated, this could take a while..." << endl;
	GraphBuilder();

	cout << "[1] Query using DFS" << endl;
	cout << "[2] Query using DFS+Caching+BloomFilter" << endl;
	cout << "[0] Exit" << endl;

	int choice;
	while (true) {
		cout << "\n-> ";
		cin >> choice;
		if (choice == 0) return 0;

		cout << "Check if a X is a factor of Y" << endl;
		int x, y;
		cout << "Enter X: ";
		cin >> x;
		cout << "Enter Y: ";
		cin >> y;

		switch (choice) {
			case 1:
				cout << x << " is " << (searchUsingDFS(x, y) ? "not a " : "") << "factor of " << y << endl;
				break;
			case 2:
				cout << x << " is " << (searchUsingBloomFilter(x, y) ? "not a " : "") << "factor of " << y << endl;
				break;
		}
	}

	return -1;
}