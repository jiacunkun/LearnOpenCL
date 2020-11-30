/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

/** @file
* @brief
*
* @author Lei Hua
* @date 2019-03-19
*/

#ifndef __ACV_TIME_COST_SUMMMARY_H__
#define __ACV_TIME_COST_SUMMMARY_H__

#include "acvutility.h"
#include <list>
#include <utility>
#include <vector>

#ifdef ENABLE_TIME_COST_SUMMARY
#define ACV_TIME_COST_SCOPED(key_word)         acv::TimeCostSummary g_scoped_time_cost_summary(key_word)
#define ACV_TIME_COST_BEGIN(key_word)          acv::TimeCostSummary::begin(key_word)
#define ACV_TIME_COST_END(key_word)            acv::TimeCostSummary::end(key_word)
#define ACV_TIME_COST_BEGIN_INDEX()            acv::TimeCostSummary::begin()  
#define ACV_TIME_COST_END_INDEX(key_word, index)    acv::TimeCostSummary::end(key_word, index)
#define ACV_TIME_COST_CLEAR()                  acv::TimeCostSummary::clearTimeCostSummary()
#define ACV_TIME_COST_RETRIEVE()               acv::TimeCostSummary::getTimeCostSummary(false)
#define ACV_TIME_COST_RETRIEVE_CLEAR()         acv::TimeCostSummary::getTimeCostSummary(true)

// more short
#define ATC_SCOPED(key_word)         acv::TimeCostSummary g_scoped_time_cost_summary(key_word)
#define ATC_BEGIN(key_word)          acv::TimeCostSummary::begin(key_word)
#define ATC_END(key_word)            acv::TimeCostSummary::end(key_word)
#define ATC_BEGIN_INDEX()            acv::TimeCostSummary::begin()  
#define ATC_END_INDEX(key_word, index)    acv::TimeCostSummary::end(key_word, index)
#define ATC_CLEAR()                  acv::TimeCostSummary::clearTimeCostSummary()
#define ATC_RETRIEVE()               acv::TimeCostSummary::getTimeCostSummary(false)
#define ATC_RETRIEVE_CLEAR()         acv::TimeCostSummary::getTimeCostSummary(true)

#else
#define ACV_TIME_COST_SCOPED(key_word) 
#define ACV_TIME_COST_BEGIN(key_word)  
#define ACV_TIME_COST_END(key_word) 
#define ACV_TIME_COST_BEGIN_INDEX()           0
#define ACV_TIME_COST_END_INDEX(key_word, index)
#define ACV_TIME_COST_CLEAR()          
#define ACV_TIME_COST_RETRIEVE()       std::string()  
#define ACV_TIME_COST_RETRIEVE_CLEAR()  std::string() 

#define ATC_SCOPED(key_word) 
#define ATC_BEGIN(key_word)  
#define ATC_END(key_word) 
#define ATC_BEGIN_INDEX()           0
#define ATC_END_INDEX(key_word, index)
#define ATC_CLEAR()          
#define ATC_RETRIEVE()       std::string()  
#define ATC_RETRIEVE_CLEAR()  std::string() 

#endif // ENABLE_TIME_COST_SUMMARY



namespace acv
{
	/**
	* TimeCostSummary is designed to help performance reporting.
	* usage example
	* @code
	*int time_lapse(int n)
	*{
	*	int sum;
	*	for (int i = 0; i < n; i++)
	*	{
	*		sum += i;
	*	}
	*	return sum;
	*}
	*
	*int TimeCostSummary()
	*{
	*	ACV_TIME_COST_CLEAR(); // clear time cost summary;
	*	{
	*		ACV_TIME_COST_SCOPED("time1");
	*		time_lapse(1000000);
	*	}
	*
	*	ACV_TIME_COST_BEGIN("time2");
	*	ACV_TIME_COST_BEGIN("time3");
	*	time_lapse(1000000);
	*	ACV_TIME_COST_END("time2");
	*
	*	time_lapse(1000000);
	*	ACV_TIME_COST_END("time3");
	*
	*	printf(ACV_TIME_COST_RETRIEVE().c_str());
	*
	*}
	*
	* @endcode
	*/

	

	class ACV_EXPORTS TimeCostSummary
	{
	public:
		TimeCostSummary(const char* key_words);
		~TimeCostSummary();

		static void begin(const char* key_words);
		static void end(const char* key_words);

		static int begin();
		static void end(const char* key_words, int index);

		static std::string getTimeCostSummary(bool is_clear = false);
		static void clearTimeCostSummary();

	private:
		double scoped_start_time_;
		double scoped_end_time_;
		std::string scoped_key_words_;

		static std::string summary_str_;
		static std::list< std::pair<std::string, double> > cost_time_list_;

		static std::vector<double> cost_time_vector_;
		static int current_index_;
	};
}

#endif // !__ACV_TIME_COST_SUMMMARY_H__


