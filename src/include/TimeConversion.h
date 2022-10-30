#pragma once

#include <algorithm>
#include <chrono>
#include <sstream>

class TimeConversion {
public:
	/// \brief parse time up to seconds granularity in format YYY-MM-DDThh:mm:ss
	/// \param [in] from string to be parsed
	/// \return encoded time point
	static std::chrono::time_point<std::chrono::system_clock> parseTimestamp(const std::string &from)
	{
		std::chrono::time_point<std::chrono::system_clock> retval = std::chrono::system_clock::now();

		if (!from.empty())
		{
			int tm_year = 0, tm_mon = 1;

			time_t rawtime;
			tm *tm1;
			time(&rawtime);
			tm1 = localtime(&rawtime);

			std::string buf(from);
			std::replace(buf.begin(), buf.end(), '-', ' ');
			std::replace(buf.begin(), buf.end(), 'T', ' ');
			std::replace(buf.begin(), buf.end(), ':', ' ');
			std::replace(buf.begin(), buf.end(), '.', ' ');

			std::istringstream is(buf);
			is >> tm_year >> tm_mon >> tm1->tm_mday >> tm1->tm_hour >> tm1->tm_min >> tm1->tm_sec;
			tm1->tm_year = tm_year - 1900;
			tm1->tm_mon = tm_mon - 1;

			time_t tt = mktime(tm1);

			if (tt >= 0)
			{
				retval = std::chrono::system_clock::from_time_t(tt);
			}
		}
		return retval;
	}

	/// \brief Encode timestamp
	/// \param [in] from timestamp to be encoded
	/// \return encoded string
	static std::string encodeTimestamp(std::chrono::time_point<std::chrono::system_clock> from)
	{
		using namespace std::chrono;

		std::string to;
		if (from.time_since_epoch() != system_clock::duration())
		{
			auto fromMs = std::chrono::duration_cast<std::chrono::milliseconds>(from.time_since_epoch()).count() % 1000;
			auto time = std::chrono::system_clock::to_time_t(from);
			// auto tm = *std::gmtime(&time);
			auto tm = *std::localtime(&time);

			char buf[80];
			strftime(buf, sizeof(buf), "%FT%T.mmm%z", &tm);
			std::string str(buf);

			// convert to ISO8601 Date (Extend) format
			std::ostringstream os;
			os.fill('0');
			os.width(3);
			os << fromMs;
			str.replace(str.find("mmm"), 3, os.str());
			str.insert(str.size() - 2, 1, ':');

			to = str;
		}
		return to;
	}
};
