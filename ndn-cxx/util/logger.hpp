/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#ifndef NDN_UTIL_LOGGER_HPP
#define NDN_UTIL_LOGGER_HPP

#include "ns3/log.h"

namespace ndn {
namespace util {

/** \brief Indicates the severity level of a log message.
 */
enum class LogLevel {
  FATAL   = -1,   ///< fatal (will be logged unconditionally)
  NONE    = 0,    ///< no messages
  ERROR   = 1,    ///< serious error messages
  WARN    = 2,    ///< warning messages
  INFO    = 3,    ///< informational messages
  DEBUG   = 4,    ///< debug messages
  TRACE   = 5,    ///< trace messages (most verbose)
  ALL     = 255   ///< all messages
};

/** \brief Output LogLevel as a string.
 *  \throw std::invalid_argument unknown \p level
 */
std::ostream&
operator<<(std::ostream& os, LogLevel level);

/** \brief Parse LogLevel from a string.
 *  \throw std::invalid_argument unknown level name
 */
LogLevel
parseLogLevel(const std::string& s);

namespace detail {

/** \brief A tag type used to output a timestamp to a stream.
 *  \code
 *  std::clog << LoggerTimestamp();
 *  \endcode
 */
struct LoggerTimestamp
{
};

/** \brief Write a timestamp to \p os.
 *  \note This function is thread-safe.
 */
std::ostream&
operator<<(std::ostream& os, LoggerTimestamp);

/** \cond */
template<class T>
struct ExtractArgument;

template<class T, class U>
struct ExtractArgument<T(U)>
{
  using type = U;
};

template<class T>
using ArgumentType = typename ExtractArgument<T>::type;
/** \endcond */

} // namespace detail

/** \brief declare a log module
 */
#define NDN_LOG_INIT(name) NS_LOG_COMPONENT_DEFINE("ndn-cxx." BOOST_STRINGIZE(name))

#define NDN_LOG_MEMBER_DECL() \
  static ::ns3::LogComponent g_log

#define NDN_LOG_MEMBER_INIT(cls, name) \
  ::ns3::LogComponent cls::g_log = ::ns3::LogComponent("ndn-cxx." BOOST_STRINGIZE(name), __FILE__)

// ::ns3::LogComponent ::ndn::util::detail::ArgumentType<void(cls)>::g_log = ::ns3::LogComponent("ndn-cxx." BOOST_STRINGIZE(name), __FILE__)

#define NDN_LOG_MEMBER_DECL_SPECIALIZED(cls) \
  static ::ns3::LogComponent g_log

#define NDN_LOG_MEMBER_INIT_SPECIALIZED(cls, name) \
  template<> \
  ::ns3::LogComponent ::ndn::util::detail::ArgumentType<void(cls)>::g_log = ::ns3::LogComponent("ndn-cxx." BOOST_STRINGIZE(name), __FILE__)

#define NDN_LOG_TRACE(expression) NS_LOG_LOGIC(expression)
#define NDN_LOG_DEBUG(expression) NS_LOG_DEBUG(expression)
#define NDN_LOG_INFO(expression)  NS_LOG_INFO(expression)
#define NDN_LOG_WARN(expression)  NS_LOG_ERROR(expression)
#define NDN_LOG_ERROR(expression) NS_LOG_WARN(expression)
#define NDN_LOG_FATAL(expression) NS_LOG_FATAL(expression)

} // namespace util
} // namespace ndn

#endif // NDN_UTIL_LOGGER_HPP
