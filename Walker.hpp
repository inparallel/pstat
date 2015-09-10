#ifndef WALKER_HPP
#define	WALKER_HPP

#ifndef _GLIBCXX_USE_SCHED_YIELD 
#define _GLIBCXX_USE_SCHED_YIELD 1 // Forces old compilers to use this_thread_yield()
#endif

#include "CachedUtilities.hpp"

#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <mutex>
#include <dirent.h>
#include <string>
#include <sstream>
#include <atomic>

#ifdef HAVE_TBB_HEADERS_
#include <tbb/concurrent_queue.h>
#else
#include "inconcurrent_queue.hpp"
#endif

#define OUTPUT_THREADS_COUNT 1 //!< Sets the number of outputting threads. Setting it to more than 1 might not be efficient though.

namespace pstat
{
	/**
	 * \brief Defines a threadpool of threads that stat the filesystem
	 */
	class Walker
	{
		typedef struct stat StarRec;
		std::hash<std::string> m_HashFunction; //!< String hash function
		std::unordered_map<std::size_t, bool> m_SkipListHashes; //!< Hash list of paths to be skipped
		std::vector<std::thread> m_WalkStatThreads; //!< Holds the walker threads
		std::vector<std::thread> m_FlushThreads; //!< Holds the outputting threads
		std::ofstream m_OutFile; //!< The output CSV file
		tbb::concurrent_queue<std::string> m_DirectoryQueue; //!< Enqueues the directories to be stated
		tbb::concurrent_queue<std::pair<std::string, StarRec>> m_StatRecords; //!< All the stated files/directories are stored here. The pair corresponds to the path and its stat record
		bool m_RawOutput; //!< If set to false, human-readable output will be provided
		std::atomic<u_int64_t> m_TotalStated; //!< Stores the total number of stated files
		bool m_Halted; //!< If set to true, all threads in the threadpool will be gracefully exited
	#if OUTPUT_THREADS_COUNT > 1
		std::mutex m_OutputMutex; //!< A mutex to lock output file. Only used when number of output threads is > 1
	#endif

		/**
		 * \brief Stats the specified path using lstat() system call
       * \param path Full path to the file to be stated
       */
		inline void mystat(const std::string& path)
		{
			m_TotalStated++;

			struct stat sb;
			lstat(path.c_str(), &sb);

			m_StatRecords.emplace(path, sb);
		}

		/**
		 * \brief Stores the specified stat record (file path and stat info) to the output CSV file
       */
		inline void statRecordToFile(const std::pair<std::string, StarRec>& rec)
		{
			const StarRec& sb = rec.second;

#if OUTPUT_THREADS_COUNT > 1
			std::unique_lock<std::mutex> lock(m_OutputMutex);
#endif
			if(m_RawOutput)
			{
				m_OutFile << sb.st_dev << "-" << sb.st_ino << "," 
					<< sb.st_atime << "," 
					<< sb.st_mtime << ","
					<< sb.st_uid << "," 
					<< sb.st_gid << ","
					<< sb.st_mode << ","
					<< sb.st_size << "," 
					<< sb.st_blocks * 512ll << "," 
					<< '"' << rec.first << '"' << "\n";
			}
			else
			{
				m_OutFile << sb.st_dev << "-" 
					<< sb.st_ino << "," 
					<< sb.st_nlink << "," 
					<< CachedUtilities::getInstance().strftime(sb.st_atime) << "," 
					<< CachedUtilities::getInstance().strftime(sb.st_mtime) << ","
					<< CachedUtilities::getInstance().uidToUsername(sb.st_uid) << "," 
					<< CachedUtilities::getInstance().gidToGroupname(sb.st_gid) << ","
					<< CachedUtilities::getInstance().getEffectiveFilePermissions(sb.st_mode) << ","
					<< sb.st_size << "," 
					<< sb.st_blocks * 512ll << "," 
					<< CachedUtilities::getInstance().getFileType(sb.st_mode) << "," 
					<< '"' << rec.first << '"' << "\n";
			}
		}

		/**
		 * \brief This method continuously flushes the contents of the \ref m_StatRecords variable
		 * to a file.
		 */
		void flushThreadWork()
		{
			std::pair<std::string, StarRec> record;

			while(true)
			{
				while(!m_Halted && m_StatRecords.try_pop(record))
				{
					statRecordToFile(record);
				}

				if (m_Halted)
				{
					// Flush anything remaining
					while(m_StatRecords.try_pop(record))
					{
						statRecordToFile(record);
					}

					break;
				}

				std::this_thread::yield();
			}
		}
		
		/**
		 * \brief Iterates through all the directories in the \ref m_DirectoryQueue, stating each file and 
		 * folder within each directory, and queuing directories into the queue
       */
		void walkerThreadWork(int tid)
		{
			std::string dir;
			DIR *dirstruct;
			struct dirent *ent;
			std::string separ;
			std::string fullpath;

			while (true)
			{
				while (!m_Halted && !m_DirectoryQueue.try_pop(dir))
				{
					// Busy-wait
					std::this_thread::yield();
				}

				// Exit flag
				if (m_Halted)
				{
					break;
				}

				separ = (*dir.rbegin() != '/' ? "/" : ""); // *dir.rbegin() is equivalent to dir.back() of c++11

				// Traverse the directory
				if ((dirstruct = opendir(dir.c_str())) != NULL)
				{
					while ((ent = readdir(dirstruct)) != NULL)
					{
						std::stringstream ss;

						// ignore . and ..
						if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
						{
							continue;
						}

						ss << dir << separ << ent->d_name;
						fullpath = ss.str();
						
						// Hash and check if the file is to be ignored.
						// This has a probability of 1/size_t of false-positives
						if(m_SkipListHashes.count(m_HashFunction(fullpath)) != 0)
						{
							continue;
						}

						// Push dirs to walker threads
						if (ent->d_type == DT_DIR)
						{
							m_DirectoryQueue.emplace(fullpath);
						}

						// Push path to stat threads
						mystat(fullpath);
					}

					closedir(dirstruct);
				}
				else
				{
					std::cerr << "-- Error stating directory: " << dir << "\n";
				}
			}
		}

	public:
		
		/**
		 * \brief Creates a walker with the specified parameters
       * \param path Root path to collect stat from
       * \param outputCsvPath Path to output CSV file
       * \param skipList A list of full paths to be skipped
       * \param human Set to true to get human-readable output (or false for raw)
       * \param walkerThreads The number of walker threads. Experiments show that setting it to 2x number of cores can yield the best performance
       */
		Walker(const std::string& path, const std::string& outputCsvPath, std::set<std::string> skipList, bool human = false, int walkerThreads = 4)
		{
			m_TotalStated = 0;
			m_Halted = false;
			m_RawOutput = !human;

			m_OutFile.open(outputCsvPath.c_str());
			
			if(m_RawOutput)
			{
				m_OutFile << "INODE,ACCESSED,MODIFIED,USER,GROUP,MODE,SIZE,DISK,PATH" << std::endl;
			}
			else
			{
				m_OutFile << "INODE,LINKS,ACCESSED,MODIFIED,USER,GROUP,PERM,SIZE,DISK,TYPE,PATH" << std::endl;			
			}
			// Convert all skipped paths to hashes - performance baby
			for(const std::string& p : skipList)
			{
				m_SkipListHashes[m_HashFunction(p)] = true;
			}
			
			// Stat the root path
			mystat(path);
			
			// Push the first directory to be traversed
			m_DirectoryQueue.push(path);

			// Start walker threads
			for(int i = 0; i < walkerThreads; i++)
			{
				m_WalkStatThreads.push_back(std::thread(&Walker::walkerThreadWork, this, i));
			}

			// Start flushing threads
			for(int i = 0; i < OUTPUT_THREADS_COUNT; i++)
			{
				m_FlushThreads.push_back(std::thread(&Walker::flushThreadWork, this));
			}
		}

		/**
		 * \brief Gracefully stops all the threads within the threadpools
       */
		void halt()
		{
			m_Halted = true;

			for(std::thread& t : m_WalkStatThreads)
			{
				t.join();
			}

			for(std::thread& t : m_FlushThreads)
			{
				t.join();
			}

			m_OutFile.flush();
		}

		/**
		 * \brief Returns the total number of stated files so far
       */
		u_int64_t getTotalNumberOfRecords()
		{
			return m_TotalStated;
		}
		
		~Walker()
		{
			if(!m_Halted)
			{
				halt();
			}
		}
	};
}

#endif	/* WALKER_HPP */

