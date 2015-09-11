#include <iostream>
#include <limits.h>
#include <set>

#include "config.h"
#include "vendor/cmdline.h"
#include "Walker.hpp"
#include "Stopwatch.hpp"

#define VERSION_MAJOR "0"
#define VERSION_MINOR "6"
#define VERSION VERSION_MAJOR "." VERSION_MINOR

/**
 * A simple method that will split the specified string by the specified delimeter,
 * returning only distinct non-empty elements after splitting
 */
std::set<std::string> split(const std::string &s, char delim)
{
	std::set<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	
	while (std::getline(ss, item, delim))
	{
		if(item.length() != 0)
		{
			elems.insert(item);
		}
	}
	
	return elems;
}

/**
 * \brief returns the resolved path of the specified path
 */
std::string resolvePath(const char* path)
{
	char buff[PATH_MAX];
	
	realpath(path, buff);
	
	return buff;
}

/**
 * \brief Returns true if the specified file exists; false otherwise 
 */
bool fileExists(const std::string& path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

int main(int argc, char** argv)
{
	//pstat::CachedUtilities::init();
	
	// <editor-fold defaultstate="collapsed" desc="Command-line args parsing">
	cmdline::parser argsParser;
	argsParser.set_program_name(argv[0]);
	argsParser.add<std::string>("output-csv", 'o', "Path to output file. If not specified, then it will be constructed "
			  "from the target stat path.", false);
	argsParser.add<int>("num-threads", 't', "Number of threads that walk the path tree. Defaults to number of cores in "
			  "the machine if not specified.", false, std::thread::hardware_concurrency(), cmdline::range(1, 1024));
	argsParser.add<unsigned long>("check-interval", 'i', "Time interval, in milliseconds, to check for parallel stat completion. "
			  "Default is 200 ms. Set it to >1000 ms if pstat fails to stat all the files within the tree of the specified path.",
			  false, 200, cmdline::range(200, 300000));
	argsParser.add<std::string>("ignore-list", 'g', "List of full paths to ignore, separated by a colon (e.g. /etc:/dev/null).", false);
	argsParser.add("human", 'h', "Displays the results in human-readable format (e.g., UIDs and GIDs are resolved to names).");
	argsParser.add("no-prompt", 'y', "Do not prompt if the specified output file exist, go ahead an overwrite.");
	argsParser.add("version", 'v', "Prints version info an exits.");
	argsParser.footer("<target stat path>");

	argsParser.parse_check(argc, argv);
	
	// Print version and exit
	if(argsParser.exist("version"))
	{
		std::cout << "pstat v" << VERSION << " - Parallel stat collector" << std::endl;
		std::cout << "Written by Mazen Abdulaziz (mazen.abdulaziz@gmail.com), 2015" << std::endl;
		std::cout << std::endl;
		
		return 0;
	}

	// Detect not passing a path
	if (argsParser.rest().size() == 0)
	{
		std::cout << "Wrong usage - path to stat is required" << std::endl;
		std::cout << argsParser.usage() << std::endl;

		return 0;
	}

	std::string path = resolvePath(argsParser.rest()[0].c_str());
	std::string outputPath = argsParser.get<std::string>("output-csv");
	std::string ignore = argsParser.get<std::string>("ignore-list");
	int numThreads = argsParser.get<int>("num-threads");
	unsigned long checkInterval = argsParser.get<unsigned long>("check-interval");
	bool human = argsParser.exist("human");
	bool noPrompt = argsParser.exist("no-prompt");
	
	std::set<std::string> ignoreList;
	
	if(ignore.length() > 0)
	{
		ignoreList = split(ignore, ':');
	}
	
	// Make sure the target path exists
	if(!fileExists(path))
	{
		std::cerr << "Error: the specified target path (" << path << ") does not exist. Aborting..." << std::endl;
		return -1;
	}
	
	if (outputPath.size() == 0)
	{
		// If no output is specified, construct the csv file name using the specified path -- replacing / with - 
		outputPath = path + ".csv";
		
		if(outputPath[0] == '/')
		{
			outputPath = outputPath.substr(1);
		}
		
		std::replace(outputPath.begin(), outputPath.end(), '/', '-');
		outputPath = resolvePath(outputPath.c_str());
	}
	else
	{
		outputPath = resolvePath(argsParser.get<std::string>("output-csv").c_str());
	}
	
	// Prompt if the file exists
	if(!noPrompt && fileExists(outputPath))
	{
		std::cout << "The specified output file (" << outputPath << ") already exists. Do you want to overwrite it? [Y/n]: ";
		char answer = std::cin.get();

		if (answer != 'Y' && answer != 'y' && answer != '\n')
		{
			std::cout << "The operation was canceled by the user." << std::endl;
			return 0;
		}
	}
	
	// </editor-fold>
	
	std::cout << "pstat v" << VERSION << " - Parallel stat collector" << std::endl;
#ifndef HAVE_TBB_HEADERS_
	std::cout << "NOTE: this version is not using Thread Building Blocks classes. You will not get the maximum performance." << std::endl;
#endif
	std::cout << std::endl;
	
	// In case number of hardware threads not detected
	if(numThreads == 0)
	{
		std::cerr << "Warning: cannot determine number of hardware threads on the system. Setting number of threads to 8." << std::endl;
		numThreads = 8;
	}
	
	std::cout << "Collecting stat from: " << path << std::endl;
	std::cout << "Number of threads: " << numThreads << std::endl;
	std::cout << "CSV output file: " << outputPath << std::endl;
	std::cout << "Check interval: " << checkInterval << " ms" << std::endl;
	std::cout << "Human output: " << (human ? "Yes" : "No") << std::endl;
	std::cout << std::endl;
	std::cout << "* Collection started" << std::endl;
	
	pstat::Stopwatch watch(true);
	pstat::Walker walker(path, outputPath, ignoreList, human, numThreads);
	
	// Convert to usec
	checkInterval *= 1000ul;
	
	size_t oldSize = 0;
	size_t newSize;
	while(true)
	{
		usleep(checkInterval);
		
		newSize = walker.getTotalNumberOfRecords();
		
		if(newSize == oldSize)
		{
			walker.halt();
			break;
		}
		
		std::cout << "-- Collected " << newSize << " stat records so far..." << std::endl;
		oldSize = newSize;
	}
	
	watch.stop();

	std::cout << "* Collection finished" << std::endl;
	std::cout << std::endl;
	std::cout << "Elapsed time: " << watch.getElapsed() << "s\n";
	std::cout << "Total files: " << walker.getTotalNumberOfRecords() << std::endl;
	std::cout << "Files/second: " << walker.getTotalNumberOfRecords() / watch.getElapsed() << std::endl;
	std::cout << std::endl;
	
	return 0;
}
