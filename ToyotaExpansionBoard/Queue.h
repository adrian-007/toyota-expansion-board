/*
 * Copyright (C) 2021 adrian_007, adrian-007 on o2 point pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#pragma once

#include <stdint.h>

template<typename T, uint8_t Capacity>
class Queue
{
public:
	bool push(T element)
	{
		if (full())
			return false;

		_elements[_end] = element;
		_end = (_end + 1) % Capacity;
		++_size;
		return true;
	}
	
	bool pop()
	{
		if (empty())
			return false;
		
		_begin = (_begin + 1) % Capacity;
		--_size;
		return true;
	}

	T* peek()
	{
		return empty() ? nullptr : &_elements[_begin];
	}

	uint8_t size() const { return _size; }
	bool empty() const { return _size == 0; }
	bool full() const { return _size == Capacity; }

private:
	T _elements[Capacity];
	
	uint8_t _size = 0;
	uint8_t _begin = 0;
	uint8_t _end = 0;
};
