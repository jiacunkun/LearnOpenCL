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

/** @brief Log system
*  Supported macros in decreasing severity level per line:
*  VLOG(2), VLOG(N)
*  LOG(DEBUG),VLOG(1),
*  LOG(INFO), VLOG(0)
*  LOG(WARNING),
*  LOG(ERROR),
*  LOG(FATAL),
*  CHECK(condition)  - fails if condition is false and logs condition. the log severity is FATAL
*  CHECK_NOTNULL(ptr) - fails if ptr is NULL. ptr can be raw pointer or smart pointer. the log severity is FATAL
*  
*  The log levels is supposed to:
*  
*  2 - Verbose
*  1 - Debug
*  0 - Info
*  -1 - Warning
*  -2 - Error
*  -3 - Fatal
*  Define MAX_LOG_LEVEL to shield some logs you don't want to output. Or call SetMaxLogLevel to change it in running time.
*  
*  To use the debug only versions, prepend a D to the normal macros, e.g.
*  DLOG(INFO) << "this is debug version INFO log";
*  DCHECK(a>b);
* @author Lei Hua
* @date 2017-11-16
*/
//#define __ACV_LOGGER_H__
//
//#define DCHECK
//#define DCHECK_NOTNULL


#ifndef __ACV_OCL_LOGGER_H__
#define __ACV_OCL_LOGGER_H__

#include "CLConfig.h"

#ifdef HAVE_ACVCORE
#include "logger.h"

#else // HAVE_ACVCORE

#include <string.h>
#include <ctime>
#include "CLConfig.h"

#include <cstddef>
#include <cstdint>
#include <stdint.h>
#include <sstream>

#define USE_STD_STRING_STREAM

namespace acv {

	namespace ocl {

		const int DEBUG = 1;
		const int INFO = 0;
		const int WARNING = -1;
		const int AERROR = -2;   // to avoid ERROR definition in wingdi.h
		const int FATAL = -3;

		/**@brief Sink for receiving log
		*/
		class ACV_EXPORTS Sink {
		public:
			virtual ~Sink() {}
			virtual void send(int severity,          // log severity                
				const struct tm* tm_time,
				const char* message,
				size_t message_len) = 0;
		};


		/**! Add a log sink. By adding a sink, all log is also sent to each sink through the Sink::send function
		@param sink User defined log sink inherited from Sink
		*/
		ACV_EXPORTS void AddLogSink(Sink* sink);

		/**! Remove a log sink.
		@param sink use defined log sink inherited from Sink
		*/
		ACV_EXPORTS void RemoveLogSink(Sink* sink);

		ACV_EXPORTS void SendToLogSink(int severity,          // log severity
			const struct tm* tm_time,
			const char* message,
			size_t message_len);


		/**! Set max log level.
		@param severity  logs whoes level is larger than severity will not output
		*/
		ACV_EXPORTS void SetMaxLogLevel(const int severity);

		ACV_EXPORTS bool NotLargerThanMaxLogLevel(const int severity);


		namespace internal {
#ifndef USE_STD_STRING_STREAM
			// try to replace std::stringstream
			class ACV_EXPORTS SimpleStream
			{
			public:
				SimpleStream();
				~SimpleStream();

				template<typename T>
				SimpleStream& operator<<(T& str)
				{
					AppendStr(str);
					return *this;
				}

				template<typename T>
				SimpleStream& operator<<(const T& str)
				{
					AppendStr(str);
					return *this;
				}

				char* c_str() const;
				size_t size() const;
			private:
				char* str_;
				static int max_size_;

				void AppendStr(const char* str);
				void AppendStr(char c);
				//void AppendStr(int n);
				//void AppendStr(unsigned int n);
				//
				//void AppendStr(bool b);
				//void AppendStr(short s);
				//void AppendStr(unsigned short s);

				/*void AppendStr(bool b);
				void AppendStr(long l); */

				void AppendStr(uint8_t u8);
				void AppendStr(int8_t i8);

				void AppendStr(uint16_t u16);
				void AppendStr(int16_t i16);

				void AppendStr(uint32_t u32);
				void AppendStr(int32_t i32);

				void AppendStr(uint64_t u64);
				void AppendStr(int64_t i64);

				//void AppendStr(size_t size);
				//void AppendStr(ptrdiff_t ptrdiff);

				void AppendStr(float f);
				void AppendStr(double d);
			};
#endif // USE_STD_STRING_STREAM

			/**@brief Output log messagers to Cerr or Android logcat when ANDOID is defined and log Sink if any
			*/
			//Implement Logger in the header file to avoid exporting Logger for DLL
			// When exporting Logger, Visual Studio reports warning C4251
			class ACV_EXPORTS Logger
			{
			public:
				Logger(const char* file, int line, int severity);

				~Logger();

				std::stringstream& stream();
				//SimpleStream &stream();

				static int max_log_level_;
			private:


#ifdef USE_STD_STRING_STREAM
				std::stringstream stream_;
#else
				SimpleStream stream_;
#endif
				int severity_;

				void LogToSinks();
				void LogToAndroid();

				void Failed();

				static char severity_flag_[5];
			};



			// ---------------------- Logging Macro definitions --------------------------

			// VoidLogger avoids compiler warnings like "value computed
			// is not used" and "statement has no effect".
			class VoidLogger {
			public:
				VoidLogger() { }
				// This has to be an operator with a precedence lower than << but
				// higher than ?:
#ifdef USE_STD_STRING_STREAM
				void operator&(const std::ostream& s) { (void)s; }
#else
				void operator&(const SimpleStream& s) { (void)s; }
#endif // USE_STD_STRING_STREAM
			};

		}; // namespace internal
	}; // namespace ocl
};  // namespace acv






#ifndef MAX_LOG_LEVEL
#define MAX_LOG_LEVEL 2
#endif // !MAX_LOG_LEVEL

#define REPORT_LOG_IF(severity, condition) \
    !(condition) ? (void) 0 : acv::ocl::internal::VoidLogger() & \
      acv::ocl::internal::Logger((char *)__FILE__, __LINE__, severity).stream()
  

#define REPORT_LOG_FATAL                REPORT_LOG_IF(acv::ocl::FATAL, acv::ocl::FATAL<=MAX_LOG_LEVEL)
#define REPORT_LOG_IF_FATAL(condition)  REPORT_LOG_IF(acv::ocl::FATAL, (acv::ocl::FATAL<=MAX_LOG_LEVEL)&&(condition))


#define REPORT_LOG_ERROR                REPORT_LOG_IF(acv::ocl::AERROR, acv::ocl::AERROR<=MAX_LOG_LEVEL)
#define REPORT_LOG_IF_ERROR(condition)  REPORT_LOG_IF(acv::ocl::AERROR, (acv::ocl::AERROR<=MAX_LOG_LEVEL)&&(condition))


#define REPORT_LOG_WARNING              REPORT_LOG_IF(acv::ocl::WARNING, (acv::ocl::WARNING<=MAX_LOG_LEVEL))
#define REPORT_LOG_IF_WARNING(condition) REPORT_LOG_IF(acv::ocl::WARNING, (acv::ocl::WARNING<=MAX_LOG_LEVEL)&&(condition))


#define REPORT_LOG_INFO                REPORT_LOG_IF(acv::ocl::INFO, (acv::ocl::INFO<=MAX_LOG_LEVEL))
#define REPORT_LOG_IF_INFO(condition)  REPORT_LOG_IF(acv::ocl::INFO, (acv::ocl::INFO<=MAX_LOG_LEVEL)&&(condition))

#define REPORT_LOG_DEBUG                REPORT_LOG_IF(acv::ocl::DEBUG, (acv::ocl::DEBUG<=MAX_LOG_LEVEL))
#define REPORT_LOG_IF_DEBUG(condition)  REPORT_LOG_IF(acv::ocl::DEBUG, (acv::ocl::DEBUG<=MAX_LOG_LEVEL)&&(condition))

// wingdi.h defines ERROR to be 0. 
// When we call LOG(ERROR), it expands to REPORT_LOG_0. 
#define REPORT_LOG_0        REPORT_LOG_ERROR
#define REPORT_LOG_IF_0     REPORT_LOG_IF_ERROR

#define LOG(severity)      REPORT_LOG_ ## severity

// Log only if condition is met.  Otherwise evaluates to void.
#define LOG_IF(severity, condition) REPORT_LOG_IF_ ## severity(condition)


#define VLOG(n) LOG_IF(INFO, n <= MAX_LOG_LEVEL)
#define VLOG_IF(n, condition) LOG_IF(INFO, (n <= MAX_LOG_LEVEL)&&condition)

#ifndef NDEBUG
#  define DLOG LOG
#  define DVLOG VLOG
#  define DLOG_IF LOG_IF
#  define DVLOG_IF VLOG_IF
#else
#  define DLOG(severity) if (false) LOG(severity)
#  define DVLOG(verboselevel) if (false) VLOG(verboselevel)
#  define DLOG_IF(severity, condition) if (false) LOG_IF(severity, condition)
#  define DVLOG_IF(verboselevel, condition) if (false) VLOG_IF(verboselevel, condition)
#endif



// ---------------------------- CHECK macros ---------------------------------

// Check for a given boolean condition.
#define CHECK(condition) LOG_IF(FATAL, !(condition)) << "Check failed: " #condition " "


#ifndef NDEBUG
// Debug only version of CHECK
#  define DCHECK(condition) CHECK(condition)
#else
// Optimized version - generates no code.
#  define DCHECK(condition) if (false) CHECK(condition)
#endif  // NDEBUG


// ---------------------------CHECK_NOTNULL macros ---------------------------

// Helpers for CHECK_NOTNULL(). Two are necessary to support both raw pointers
// and smart pointers.

namespace acv {
	namespace ocl {

template <typename T>
T* CheckPtrNotNull(T* t, const char *file, int line, const char *message) {
	if (t == NULL) {
		acv::ocl::internal::Logger(file, line, acv::ocl::FATAL).stream() << message;
	}
	return t;
}

template <typename T>
T& CheckPtrNotNull(T& t, const char *file, int line, const char *message) {
	if (t == NULL) {
		acv::ocl::internal::Logger(file, line, acv::ocl::FATAL).stream() << message;
	}
	return t;
}
	}
}
// Check that a pointer is not null.
#define CHECK_NOTNULL(val) \
  acv::ocl::CheckPtrNotNull((val), __FILE__, __LINE__, "'" #val "' must be non NULL" )

#ifdef _DEBUG
// Debug only version of CHECK_NOTNULL
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#else
// Optimized version - generates no code.
#define DCHECK_NOTNULL(val) if (false) CHECK_NOTNULL(val)
#endif  // NDEBUG

#endif // HAVE_ACVCORE

#endif  // _ACV_OCL_LOGGER_H__
