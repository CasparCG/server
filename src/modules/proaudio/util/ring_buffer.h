/*
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
 */

#pragma once

#include <common/audio_ring_buffer.h>
#include <cstdint>

namespace caspar { namespace proaudio {

// Single-producer/single-consumer lock-free ring buffer of interleaved audio frames.
//
// push() and pop() never block or take a lock against each other, so each may safely be called
// from a realtime PortAudio callback that must never block, allocate, or touch a mutex - which
// side that is depends on the direction of data flow: the consumer pushes from its executor
// thread and pops from its realtime playback callback, while the producer is the other way
// around (pushes from its realtime capture callback, pops from the mixer's executor thread).
// Either way, push() must only ever be called from one fixed thread and pop() from the other.
//
// One "frame" here is `channels` interleaved int16_t samples.
using frame_ring_buffer = caspar::audio_ring_buffer<std::int16_t>;

}} // namespace caspar::proaudio
