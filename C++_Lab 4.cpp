#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <string> 


std::mutex mutex;
std::condition_variable conditionVar;
std::queue<std::pair<int, float>> orders;
const int maxBufferSize = 5;
bool stopFlag = false;



void produce(int id)
{
	{ // Lock scope
		std::unique_lock<std::mutex> lock(mutex);
		conditionVar.wait(lock, [&id]() {
			// If buffer not full, continue producing
			return orders.size() < maxBufferSize; });

		orders.push(std::make_pair(id, 4));//rand() % 4));
		std::cout << "Customer " << id << " placed order: " << id << std::endl;
	}
	conditionVar.notify_one();
}

void consume(int id)
{
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		int progress = 0;
		std::pair<int, float> currentOrder;
	
		{ // Lock scope
			std::unique_lock<std::mutex> lock(mutex);
			conditionVar.wait(lock, [&id]() {	
				//std::cout << "Consumer " << id << "in waiting..." << std::endl;
				return orders.size() > 0 || stopFlag;
			});

			if (orders.size() == 0)
				break;

			currentOrder = orders.front();
			orders.pop();
		}
		conditionVar.notify_one();

		
		while (progress != currentOrder.second)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			++progress;
			std::lock_guard<std::mutex> lock(mutex);
	
			if (progress != currentOrder.second)
			{			
				std::cout << "Barista " << id << " - Order "
					<< currentOrder.first << " progress: " << ((static_cast<float>(progress) / currentOrder.second) * 100) << "%" << std::endl;
			}
			else
			{
				std::cout << "Barista " << id << " finished order " << currentOrder.first << std::endl;
			}
		}
	}
}


int main()
{
	std::vector<int> vec(20, 0);
	

#pragma region 1 - Thread Basics
#pragma region 1.1
	// Synchronization is not needed here because there is only one thread accessing the shared resourse (vec).
	// Join forces the main thread to wait for the execution of the worker thread to finish before proceding.

	std::cout << "vector before work: ";
	for (auto i : vec)
		std::cout << i << " ";
	std::cout << "\n";

	std::thread workerThread([&vec]()
		{
			for (auto& i : vec)
			{
				i = 1;
			}
		});

	workerThread.join();

	std::cout << "vector after work:  ";
	for (auto i : vec)
		std::cout << i << " ";
	std::cout << "\n";
#pragma endregion

#pragma region 1.2
	// Strictly speaking vec is a shared resource but since the task says each worker thread should
	// only have access to each own range of elements within synchronization is not needed.

	// Reset vec to zeros
	for (auto& i : vec)
	{
		i = 0;
	}
	std::cout << "\nvector before work: ";
	for (auto i : vec)
		std::cout << i << " ";
	std::cout << "\n";

	const int N = 4;
	std::thread workerThreads[N];
	int range = vec.size() / N;

	for (int i = 0; i < N; ++i)
	{
		int startIndex = i * range;
		workerThreads[i] = std::thread([&vec, startIndex, i, range]()
			{
				int index = startIndex;
				for (int j = 0; j < range; ++j)
				{
					vec[index++] = i;
				}
			});
	}

	for (int i = 0; i < N; ++i)
		workerThreads[i].join();

	std::cout << "vector after work:  ";
	for (auto i : vec)
		std::cout << i << " ";
	std::cout << "\n";
#pragma endregion
#pragma endregion

#pragma region 2 - Synchronization exercises
#pragma region 2.1
	std::cout << "Assignment 2.1: " << std::endl;
	{
		std::thread workerThreads[10];
		for (int id = 0; id < 10; ++id)
		{
			auto printThread = [](int id) {
				std::cout << "Hello" << " from " << " thread " << id << "!" << std::endl; };

			workerThreads[id] = std::thread(printThread, id);
		}

		for (int i = 0; i < 10; ++i)
			workerThreads[i].join();
	}
#pragma endregion

#pragma region 2.2
	std::cout << "\nAssignment 2.2: " << std::endl;
	{
		std::thread workerThreads[10];
		for (int id = 0; id < 10; ++id)
		{
			auto printThread = [](int id)
				{
					mutex.lock();
					std::cout << "Hello" << " from " << " thread " << id << "!" << std::endl;
					mutex.unlock();
				};

			workerThreads[id] = std::thread(printThread, id);
		}

		for (int i = 0; i < 10; ++i)
			workerThreads[i].join();
	}
#pragma endregion

#pragma region 2.3
	std::cout << "\nAssignment 2.3: " << std::endl;
	{
		std::thread workerThreads[10];
		for (int id = 0; id < 10; ++id)
		{
			auto printThread = [](int id)
				{
					std::lock_guard<std::mutex> lock(mutex);
					std::cout << "Hello" << " from " << " thread " << id << "!" << std::endl;
					// Realeses the lock when scope is exited, no need to unlock.
				};

			workerThreads[id] = std::thread(printThread, id);
		}

		for (int i = 0; i < 10; ++i)
			workerThreads[i].join();
	}
#pragma endregion
#pragma endregion

#pragma region 3 - Multithreaded searching exercises
#pragma region 3.1
	std::cout << "\nAssignment 3.1: " << std::endl;
	{
		int availableThreads = std::thread::hardware_concurrency();
		std::cout << "Number of available threads: " << availableThreads << std::endl;

		const int numThreads = 12;
		std::thread workerThreads[numThreads];
		std::cout << "Number of threads used for searching, N = " << numThreads << std::endl;

		const std::string wordToSearch = "the";
		std::string line;
		std::vector<std::string> lines{};
		std::ifstream textFile("bible.txt");

		while (std::getline(textFile, line))
		{
			if (line != "")
			{
				lines.push_back(line);
			}
		}

		auto searchText = [](const std::vector<std::string>& text, const std::string wordToSearch,
			const int linesToSearch, const int startIndex)
			{
				int index = startIndex;
				int occurrenses = 0;
				auto begin = std::chrono::high_resolution_clock::now();

				for (int i = 0; i < linesToSearch; ++i)
				{
					std::string currentLine = text[index++];
					int result = 0;

					while (true)
					{
						int offset = result == 0 ? 0 : result + 1;
						result = currentLine.find(wordToSearch, offset);
						if (result == std::string::npos || result == 0)
							break;
						else
							++occurrenses;
					}
				}

				auto finished = std::chrono::high_resolution_clock::now();
				double timer = std::chrono::duration_cast<std::chrono::milliseconds>(finished - begin).count();
				std::cout << "\nThread ID: " << std::this_thread::get_id()
					<< " searched " << linesToSearch << " lines (startIndex: " << startIndex << ") and found " << occurrenses << " occurrenses of '"
					<< wordToSearch << "' in " << timer << "ms" << std::endl;
			};


		int numLines = lines.size();
		// If the division between numLines and N is not even
		// add the remainder to the first partitions.
		int normalRange = numLines / numThreads;
		int rangeRemainder = numLines % numThreads;
		int extraRange = normalRange + 1;
		int startIndex = 0;

		for (int i = 0; i < numThreads; ++i)
		{
			if (i < rangeRemainder)
			{
				startIndex = i * extraRange;
				workerThreads[i] = std::thread(searchText, lines, wordToSearch, extraRange, startIndex);
			}
			else
			{
				startIndex = i * normalRange;
				workerThreads[i] = std::thread(searchText, lines, wordToSearch, normalRange, startIndex);
			}
		}

		for (int i = 0; i < numThreads; ++i)
			workerThreads[i].join();
	}
#pragma endregion

#pragma region 3.2
	std::cout << "\nAssignment 3.2: " << std::endl;
	{
		//Shared resources, timer and counter
		double totalTime = 0;
		double totalOccurences = 0;

		int availableThreads = std::thread::hardware_concurrency();
		std::cout << "Number of available threads: " << availableThreads << std::endl;

		const int numThreads = 2;
		std::thread workerThreads[numThreads];
		std::cout << "Number of threads used for searching, N = " << numThreads << std::endl;

		const std::string wordToSearch = "apple";
		std::string line;
		std::vector<std::string> lines{};
		std::ifstream textFile("bible.txt");

		while (std::getline(textFile, line))
		{
			if (line != "")
			{
				lines.push_back(line);
			}
		}

		auto searchText = [&totalTime, &totalOccurences](const std::vector<std::string>& text, const std::string wordToSearch,
			const int linesToSearch, const int startIndex)
			{
				int index = startIndex;
				int occurrenses = 0;
				auto begin = std::chrono::high_resolution_clock::now();

				for (int i = 0; i < linesToSearch; ++i)
				{
					std::string currentLine = text[index++];
					int result = 0;

					while (true)
					{
						int offset = result == 0 ? 0 : result + 1;
						result = currentLine.find(wordToSearch, offset);
						if (result == std::string::npos || result == 0)
							break;
						else
							++occurrenses;
					}
				}

				auto finished = std::chrono::high_resolution_clock::now();
				double timer = std::chrono::duration_cast<std::chrono::milliseconds>(finished - begin).count();

				// Modify shared resources (counter and timer)
				std::lock_guard<std::mutex> lock(mutex);
				totalOccurences += occurrenses;
				totalTime += timer;
			};


		int numLines = lines.size();
		// If the division between numLines and N is not even
		// add the remainder to the first partitions.
		int normalRange = numLines / numThreads;
		int rangeRemainder = numLines % numThreads;
		int extraRange = normalRange + 1;
		int startIndex = 0;

		for (int i = 0; i < numThreads; ++i)
		{
			if (i < rangeRemainder)
			{
				startIndex = i * extraRange;
				workerThreads[i] = std::thread(searchText, lines, wordToSearch, extraRange, startIndex);
			}
			else
			{
				startIndex = i * normalRange;
				workerThreads[i] = std::thread(searchText, lines, wordToSearch, normalRange, startIndex);
			}
		}

		for (int i = 0; i < numThreads; ++i)
			workerThreads[i].join();

		std::cout << "\n" << numThreads << " worker threads found " << totalOccurences
			<< " occurenses of the word '" << wordToSearch << "' in the text." << std::endl;
		std::cout << "The total search time was: " << totalTime << "ms." << std::endl;
	}
#pragma endregion
#pragma endregion

#pragma region 4 - Producer Consumer Simulation
		const int numProducers  = 5;     // Customers
		const int numConsumers  = 2;     // Baristas
		std::thread producers[numProducers];
		std::thread consumers[numConsumers];


		// Activate worker threads
		for (int i = 0; i < numProducers; ++i)
		{
			producers[i] = std::thread(produce, i + 1);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		for (int i = 0; i < numConsumers; ++i)
		{
			consumers[i] = std::thread(consume, i + 1);
		}


		// Join worker threads
		for (int i = 0; i < numProducers; ++i)
		{
			producers[i].join();
		}

		mutex.lock();
		stopFlag = true;
		mutex.unlock();

		for (int i = 0; i < numConsumers; ++i)
		{
			consumers[i].join();
		}
	
#pragma endregion
}