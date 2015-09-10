#ifndef STOPWATCH_H
#define	STOPWATCH_H

#include <chrono>

namespace pstat
{
	namespace cn = std::chrono;
	
	/**
	 * \brief A stopwatch class, useful for profiling
	 */
	class Stopwatch
	{
		cn::time_point<cn::system_clock> m_Start; //!< Stores the start timepoint
		cn::duration<double> m_Elapsed; //!< Stores the elapsed time

	public:

		/**
		 * \breif Creates a stopwarch. If start is set to true, the stopwatch will be started.
       */
		Stopwatch(bool start = false)
		{
			if(start)
			{
				this->start();
			}
		}
		
		/**
		 * \brief Starts the stopwatch
       */
		void start()
		{
			m_Start = cn::system_clock::now();
		}

		/**
		 * \brief Stops the stop watch
       */
		void stop()
		{
			m_Elapsed = cn::system_clock::now() - m_Start;
		}

		/**
		 * \brief Gets the total elapsed time in seconds between \ref start() and \ref stop() calls.
		 * Make sure to call \ref stop() before calling this method.
       */
		double getElapsed()
		{
			return m_Elapsed.count();
		}
	};
}

#endif	/* STOPWATCH_H */

