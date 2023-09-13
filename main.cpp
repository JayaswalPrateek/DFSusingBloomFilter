// https://hur.st/bloomfilter/?n=100&p=&m=400&k=4
#include <array>
#include <bitset>
#include <forward_list>
#include <iostream>
#include <set>
#include <stack>
#include <vector>
using namespace std;

const int TOTAL_NODES = 100000;
const int BITSET_SIZE = 400;
const int TABLE_ROW_RANGE = 100;
const int CACHE_SIZE_LIMIT = TABLE_ROW_RANGE / 10;

typedef forward_list<string> cacheLL;
typedef array<bitset<BITSET_SIZE>, 4> bloomFilter;

vector<vector<int>> adjacencyList;
vector<tuple<int, int, cacheLL, cacheLL, bloomFilter>> table(TOTAL_NODES / TABLE_ROW_RANGE);
// vector<tuple<FROM, TO, SUCCESSES, FAILURES, BLOOM_FILTER>> table(TOTAL_NODES / TABLE_ROW_RANGE);
// Indexing:   <0>	 <1> <2>		<3>		  <4>

int hashFunctions(const int functionNumber, const int toBeHashed) {
	const auto hashFunction1 = [](const int input) -> int { return (17 * input + 11) % BITSET_SIZE; };
	const auto hashFunction2 = [](const int input) -> int { return (23 * input + 7) % BITSET_SIZE; };
	const auto hashFunction3 = [](const int input) -> int { return (31 * input + 13) % BITSET_SIZE; };
	const auto hashFunction4 = [](const int input) -> int { return (41 * input + 19) % BITSET_SIZE; };
	switch (functionNumber) {
		case 1:
			return hashFunction1(toBeHashed);
		case 2:
			return hashFunction2(toBeHashed);
		case 3:
			return hashFunction3(toBeHashed);
		case 4:
			return hashFunction4(toBeHashed);
	}
	return -1;
}

bloomFilter BloomFilterBuilder(int tableRow) {
	bloomFilter bf;

	for (int i = TABLE_ROW_RANGE * tableRow; i < TABLE_ROW_RANGE * (tableRow + 1); i++)
		for (const int &neighbour: adjacencyList[i]) {
			bf[0].set(hashFunctions(1, neighbour));
			bf[1].set(hashFunctions(2, neighbour));
			bf[2].set(hashFunctions(3, neighbour));
			bf[3].set(hashFunctions(4, neighbour));
		}

	return bf;
}
void TableBuilder() {
	for (int i = 0; i < TOTAL_NODES / TABLE_ROW_RANGE; i++) {
		int from = TABLE_ROW_RANGE * i;
		int to = min(TABLE_ROW_RANGE * (i + 1), TOTAL_NODES);
		table.push_back(make_tuple(from, to, cacheLL(), cacheLL(), BloomFilterBuilder(i)));
		// cout << "FROM: " << from << " TO: " << to << endl;
	}
}
void GraphBuilder() {
	adjacencyList.resize(TOTAL_NODES + 1);

	// Building the uncompressed graph
	for (int i = 2; i <= TOTAL_NODES; i++)
		for (int j = 2; j < i; j++)
			if (i % j == 0) adjacencyList[i].push_back(j);

	TableBuilder();	 // requires uncompressed graph

	// Compressing the graph inplace
	for (int i = TOTAL_NODES; i >= 2; i--) {
		vector<int> &listToBeCompressed = adjacencyList[i];
		for (int j = listToBeCompressed.size() - 1; j >= 0; j--) {
			int largerNumber = listToBeCompressed[j];
			set<int> factorsOfLargerElement(adjacencyList[largerNumber].cbegin(), adjacencyList[largerNumber].cend());

			for (int k = listToBeCompressed.size() - 1; k >= 0; k--)
				if (factorsOfLargerElement.count(listToBeCompressed[k]))
					listToBeCompressed.erase(listToBeCompressed.cbegin() + k);	// Removed the common factors
		}
	}

	// Printing the compressed graph
	for (int i = 2; i <= TOTAL_NODES; i++) {
		cout << "Factors of " << i << " : ";
		for (const int &factor: adjacencyList[i]) cout << factor << " ";
		cout << endl;
	}
	cout << "\n\nGraph built successfully!" << endl;
}

string generateKey(const int isThisNumber, const int aFactorOfThisNumber) { return to_string(isThisNumber) + ',' + to_string(aFactorOfThisNumber); }

void pruneCache(cacheLL &cacheList) {
	if (const int cacheSize = distance(cacheList.cbegin(), cacheList.cend()); cacheSize > CACHE_SIZE_LIMIT) {
		auto justBeforeTail = next(cacheList.cbegin(), cacheSize - 1);
		cacheList.erase_after(justBeforeTail);	// deletes the tail
	}
}
void cacher(const int isThisNumber, const int aFactorOfThisNumber, const bool result) {
	const string key = generateKey(isThisNumber, aFactorOfThisNumber);
	int tableRow = isThisNumber / TABLE_ROW_RANGE;
	if (result) {
		cacheLL &cacheList = get<2>(table[tableRow]);
		cacheList.push_front(key);
		pruneCache(cacheList);
	} else {
		cacheLL &cacheList = get<3>(table[tableRow]);
		cacheList.push_front(key);
		pruneCache(cacheList);
	}
}
bool inCache(const int isThisNumber, const int aFactorOfThisNumber) {
	const string key = generateKey(isThisNumber, aFactorOfThisNumber);
	int tableRow = isThisNumber / TABLE_ROW_RANGE;
	for (const string &successfulQueries: get<2>(table[tableRow]))
		if (key == successfulQueries) return true;
	for (const string &failedQueries: get<3>(table[tableRow]))
		if (key == failedQueries) return true;
	return false;
}
bool queryCache(const int isThisNumber, const int aFactorOfThisNumber) {
	const string key = generateKey(isThisNumber, aFactorOfThisNumber);
	int tableRow = isThisNumber / TABLE_ROW_RANGE;
	for (const string &successfulQueries: get<2>(table[tableRow]))
		if (key == successfulQueries) return true;
	for (const string &failedQueries: get<3>(table[tableRow]))
		if (key == failedQueries) return false;
	exit(-1);
}

bool searchUsingDFS(const int isThisNumber, const int aFactorOfThisNumber) {
	stack<int> dfsStack;
	dfsStack.push(isThisNumber);
	vector<bool> visited(TOTAL_NODES + 1, false);

	while (not dfsStack.empty()) {
		int currentNode = dfsStack.top();
		if (currentNode == aFactorOfThisNumber) {
			cacher(isThisNumber, aFactorOfThisNumber, true);
			return true;
		}
		dfsStack.pop();
		visited[currentNode] = true;
		for (const int &factorOfCurrentNode: adjacencyList[currentNode])
			if (not visited[factorOfCurrentNode])
				dfsStack.push(factorOfCurrentNode);
	}

	cacher(isThisNumber, aFactorOfThisNumber, false);
	return false;
}

bool searchUsingBloomFilter(const int isThisNumber, const int aFactorOfThisNumber) {
	if (inCache(isThisNumber, aFactorOfThisNumber)) return queryCache(isThisNumber, aFactorOfThisNumber);

	int tableRow = isThisNumber / TABLE_ROW_RANGE;
	bloomFilter bf = get<4>(table[tableRow]);

	float p = 0;
	if (bf[0][hashFunctions(1, aFactorOfThisNumber)]) p += 0.25;
	if (bf[1][hashFunctions(2, aFactorOfThisNumber)]) p += 0.25;
	if (bf[2][hashFunctions(3, aFactorOfThisNumber)]) p += 0.25;
	if (bf[3][hashFunctions(4, aFactorOfThisNumber)]) p += 0.25;

	if (p == 0) {
		cacher(isThisNumber, aFactorOfThisNumber, false);
		return false;
	}
	return searchUsingDFS(isThisNumber, aFactorOfThisNumber);
}

void benchmarkBFS() {}
void benchmarkBloomFilter();

int main() {
	cout << "Please wait while the graph of " << TOTAL_NODES << " nodes is being generated, this could take a while..." << endl;
	GraphBuilder();

	cout << "[1] Query using DFS" << endl;
	cout << "[2] Query using DFS+Caching+BloomFilter" << endl;
	cout << "[3] Profile [1]" << endl;
	cout << "[4] Profile [2]" << endl;
	cout << "[0] Exit" << endl;

	int choice;
	while (true) {
		cout << "\n-> ";
		cin >> choice;
		if (choice == 0) return 0;

		cout << "Check if a number X is a factor of Y" << endl;
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
			case 3:
				break;
			case 4:
				break;
		}
	}

	return -1;
}