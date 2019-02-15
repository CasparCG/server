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

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/distance.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/timer.hpp>
#include <boost/variant.hpp>
#include <common/array.h>
#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/enum_class.h>
#include <common/env.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/forward.h>
#include <common/future.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/memshfl.h>
#include <common/os/filesystem.h>
#include <common/prec_timer.h>
#include <common/timer.h>
#include <common/tweener.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
