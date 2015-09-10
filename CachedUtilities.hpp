#ifndef CACHEDUTILITIES_H
#define	CACHEDUTILITIES_H

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <sstream>
#include <ctime>
#include <pwd.h>
#include <grp.h>

#ifdef HAVE_TBB_HEADERS_
#include <tbb/concurrent_unordered_map.h>
#else
#include "inconcurrent_unordered_map.hpp"
#endif

#define USER_BUF_SIZE (sysconf(_SC_GETPW_R_SIZE_MAX))

namespace pstat
{
	/**
	 * \brief A singleton that contains a set of utility methods that make use of caching for performance.
	 * All the methods are thread-safe.
	 */
	class CachedUtilities
	{
		tbb::concurrent_unordered_map<gid_t, std::string> m_CachedGroups; //!< A hashtable from GID to group name
		tbb::concurrent_unordered_map<uid_t, std::string> m_CachedUsers; //!< A hashtable from UID to username
		tbb::concurrent_unordered_map<time_t, std::string> m_CachedTimes; //!< A hashtable from timestamp to date string

		std::unordered_map<mode_t, std::string> m_CachedTypes; //!< A hashtable of file types
		std::unordered_map<mode_t, std::string> m_CachedPermissions; //!< A hashtable of file permissions
		
		/**
		 * \brief Private constructor
       */
		CachedUtilities()
		{
			// Enumerate file types
			m_CachedTypes[S_IFBLK] = "BDEV";
			m_CachedTypes[S_IFCHR] = "CDEV";
			m_CachedTypes[S_IFDIR] = "DIR";
			m_CachedTypes[S_IFIFO] = "PIPE";
			m_CachedTypes[S_IFLNK] = "LINK";
			m_CachedTypes[S_IFREG] = "FILE";
			m_CachedTypes[S_IFSOCK] = "SOCK";

			for(int i = 0; i <= 0777; i++)
			{
				std::stringstream ss;
				int o = i & 0007;
				int g = (i & 0070) >> 3;
				int u = (i & 0700) >> 6;

				ss << u << g << o;

				m_CachedPermissions[i] = ss.str();
			}
		}

	public:
		
		/**
		 * \brief Converts the specified time to sting of format YYYY-mm-dd
       */
		std::string strftime(time_t time)
		{
			struct tm timeob;
			localtime_r(&time, &timeob);

			// Rewind to midnight
			timeob.tm_sec = timeob.tm_min = timeob.tm_hour = 0;
			time_t midnight = mktime(&timeob);

			if (m_CachedTimes.count(midnight) != 0)
			{
				return m_CachedTimes[midnight];
			}

			char buf[16];
			std::strftime(buf, 16, "%Y-%m-%d", &timeob);
			std::string ret(buf);

			m_CachedTimes[midnight] = ret;

			return ret;
		}

		/**
		 * \brief Resolves the specified uid to its corresponding username in the passwd file. 
		 * Retuns "UNKNOWN" if failed to resolve.
       */
		std::string uidToUsername(uid_t uid)
		{
			if(m_CachedUsers.count(uid) != 0)
			{
				return m_CachedUsers[uid];
			}

			char buf[USER_BUF_SIZE];
			struct passwd pwd;
			struct passwd* pwdbuf;

			if(getpwuid_r(uid, &pwd, buf, sizeof(buf), &pwdbuf) != 0)
			{
				sprintf(buf, "UNKNOWN");
			}

			std::string ret(buf);
			m_CachedUsers[uid] = buf;

			return ret;
		}

		/**
		 * \brief Resolves the specified gid to its corresponding group name in the passwd file. 
		 * Retuns "UNKNOWN" if failed to resolve.
       */
		std::string gidToGroupname(gid_t gid)
		{
			if(m_CachedGroups.count(gid) != 0)
			{
				return m_CachedGroups[gid];
			}

			char buf[USER_BUF_SIZE];
			struct group grp;
			struct group* grpbuf;

			if(getgrgid_r(gid, &grp, buf, sizeof(buf), &grpbuf) != 0)
			{
				sprintf(buf, "UNKNOWN");
			}

			std::string ret(buf);
			m_CachedGroups[gid] = ret;

			return ret;
		}

		/**
		 * \brief Gets a string representing the file type from its mode.
		 * Returns one of the following types:
		 * - BDEV
		 * - CDEV
		 * - DIR
		 * - PIPE
		 * - LINK
		 * - FILE
		 * - SOCK
		 * 
		 * Returns UNKNOWN if the type cannot be resolved
       */
		std::string getFileType(mode_t mode)
		{
			unsigned int rmode = mode & S_IFMT;

			if(m_CachedTypes.count(rmode) != 0)
			{
				return m_CachedTypes[rmode];
			}

			return "UNKNOWN";
		}
		
		/**
		 * \brief Returns a string representing the effective file permission from the specified file mode
		 */
		inline std::string getEffectiveFilePermissions(mode_t mode, unsigned int parentMode = 0777)
		{
			return m_CachedPermissions[mode & parentMode];
		}
		
		/**
		 * \brief Gets the singleton instance of this class
       */
		static CachedUtilities& getInstance()
		{
			static CachedUtilities instance;

			return instance;
		}

		/**
		 * \brief Call at main to avoid race-condition when calling getInstance()
		 */
		static void init()
		{
			CachedUtilities& instance = CachedUtilities::getInstance();
		}
	};
}

#endif	/* CACHEDUTILITIES_H */

