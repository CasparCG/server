/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include <algorithm>
#include <atomic>
//#include <boost/algorithm/string.hpp>
//#include <boost/algorithm/string/replace.hpp>
//#include <boost/any.hpp>
//#include <boost/archive/iterators/base64_from_binary.hpp>
//#include <boost/archive/iterators/binary_from_base64.hpp>
//#include <boost/archive/iterators/insert_linebreaks.hpp>
//#include <boost/archive/iterators/remove_whitespace.hpp>
//#include <boost/archive/iterators/transform_width.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
//#include <boost/function.hpp>
//#include <boost/iterator/iterator_facade.hpp>
//#include <boost/lambda/lambda.hpp>
//#include <boost/lexical_cast.hpp>
//#include <boost/locale.hpp>
//#include <boost/log/attributes/attribute_value.hpp>
//#include <boost/log/attributes/function.hpp>
//#include <boost/log/core.hpp>
//#include <boost/log/core/record.hpp>
//#include <boost/log/expressions.hpp>
//#include <boost/log/sinks/async_frontend.hpp>
//#include <boost/log/sinks/sync_frontend.hpp>
//#include <boost/log/sinks/text_file_backend.hpp>
//#include <boost/log/sinks/text_ostream_backend.hpp>
//#include <boost/log/sources/global_logger_storage.hpp>
//#include <boost/log/sources/severity_channel_logger.hpp>
//#include <boost/log/trivial.hpp>
//#include <boost/log/utility/setup/common_attributes.hpp>
#include <GL/glew.h>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/join.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tbb/concurrent_queue.h>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
