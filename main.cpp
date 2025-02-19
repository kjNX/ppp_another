#include <atomic>
#include <cstdio>
#include <barrier>
#include <fstream>
#include <future>
#include <random>
#include <thread>

constexpr const char* const& DATA_PATH{""};
const unsigned& SET_SIZE{std::thread::hardware_concurrency()};

std::atomic_bool g_shouldStop{false};
int* g_array;

std::random_device g_device{};
std::seed_seq g_seq{g_device(), g_device(), g_device(), g_device()};
std::ranlux24 g_gen{g_seq};
std::uniform_int_distribution g_dist{1, 12};

// signals a stop when the file is read
std::string readFile()
{
	std::string temp{"Failed to read file!"};
	// if(std::ifstream in{DATA_PATH, std::ios::ate}; in.is_open()) std::getline(in, temp, '\0'); // hdds will screw this one up, enable at your own risk :)
	// std::this_thread::sleep_for(std::chrono::milliseconds(3)); // fast run
	std::this_thread::sleep_for(std::chrono::seconds(3)); // slow run
	g_shouldStop = true;
	return temp;
}

void makeArray()
{
	g_array = new int[SET_SIZE]{};
	std::uniform_int_distribution dist{1, 5};
	for(unsigned i{0u}; i < SET_SIZE; ++i) g_array[i] = dist(g_gen);
}

void postPass() noexcept
{
	printf("Finished iteration!\n");
	for(unsigned i{0u}; i < SET_SIZE; ++i) printf("%d ", g_array[i]);
	printf("\n");
}
std::barrier g_stopPoint{SET_SIZE}; // first barrier
std::barrier g_barrier{SET_SIZE, postPass}; // second barrier (printer barrier)

void updateArray(const int& set)
{
	int lowEnd{set - 1};
	int highEnd{set + 1};
	if(!set) lowEnd = static_cast<int>(SET_SIZE) - 1;
	else if(set == SET_SIZE - 1) highEnd = 0;

	while(!g_shouldStop)
	{
		const int& temp{g_array[highEnd] + g_array[lowEnd] - g_array[set]};
		g_stopPoint.arrive_and_wait(); // wait until all of them have computed their data
		g_array[set] = temp ? std::abs(temp) : 1;
		g_barrier.arrive_and_wait(); // update the entire array, then print it
	}
	printf("Thread %d has finished!\n", set);
}

int main()
{
	printf("%s\n", "Hello World!");
	makeArray();
	postPass();
	std::future async{std::async(&readFile)}; // threads will still run because it's async
	std::jthread threads[SET_SIZE];
	for(unsigned i{0u}; i < SET_SIZE; ++i) threads[i] = std::jthread{&updateArray, i};
	async.wait(); // wait til we get the data
	for(unsigned i{0u}; i < SET_SIZE; ++i) threads[i].join();
	printf("Finished loading file:\n%s\n", async.get().c_str());
	delete[] g_array; // cleanup ;)
	return 0;
}