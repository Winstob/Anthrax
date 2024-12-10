/* ---------------------------------------------------------------- *\
 * ringbuffer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-08
\* ---------------------------------------------------------------- */
#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <fstream>
#include <vector>

namespace Anthrax
{

template <class T>
class Ringbuffer
{
public:
	Ringbuffer<T>(int size_);
	Ringbuffer<T>() : Ringbuffer<T>(1) {}
	Ringbuffer<T> &operator=(const Ringbuffer<T> &other) { copy(other); return *this; }
	Ringbuffer<T>(const Ringbuffer<T> &other) { copy(other); }
	void resize(size_t new_size);
	void copy(const Ringbuffer<T> &other);
	void push_back(T value);
	T& operator[](size_t i);
	size_t size() { return size_; }

private:
	std::vector<T> data_;
	size_t size_;
	size_t max_size_;
	size_t start_, end_;
};


template <class T>
Ringbuffer<T>::Ringbuffer(int size)
{
	if (size < 1)
	{
		throw std::runtime_error("Ringbuffer size must be at least 1!");
	}
	max_size_ = size;
	start_ = 0;
	end_ = 0;
	size_ = 0;
	data_.resize(max_size_);
	return;
}


template <class T>
void Ringbuffer<T>::resize(size_t new_size)
{
	if (new_size < 1)
	{
		throw std::runtime_error("Ringbuffer size must be at least 1!");
	}

	// preserve elements
	std::vector<T> copy_staging_buffer(size_);
	copy_staging_buffer.resize(size_);
	start_ = 0;
	end_ = 0;
	for (unsigned int i = 0; i < size_; i++)
	{
		copy_staging_buffer[i] = (*this)[i];
		end_++;
	}
	if (end_ != 0)
	{
		end_--;
	}

	// resize and copy elements
	max_size_ = new_size;
	data_.resize(max_size_);
	if (max_size_ < size_)
	{
		size_ = max_size_;
		end_ = size_-1;
	}
	for (unsigned int i = 0; i < size_; i++)
	{
		data_[i] = copy_staging_buffer[i];
	}

	copy_staging_buffer.clear();
	return;
}


template <class T>
void Ringbuffer<T>::copy(const Ringbuffer<T> &other)
{
	max_size_ = other.max_size_;
	data_.resize(max_size_);
	size_ = other.size_;
	for (unsigned int i = 0; i < max_size_; i++)
	{
		data_[i] = other.data_[i];
	}
	start_ = other.start_;
	end_ = other.end_;
	return;
}


template <class T>
void Ringbuffer<T>::push_back(T value)
{
	if (size_ != 0)
	{
		end_ = (end_ + 1) % max_size_;
	}
	if (size_ == max_size_)
	{
		start_ = (start_ + 1) % max_size_;
	}
	else
	{
		size_++;
	}
	data_[end_] = value;
	return;
}


template <class T>
T& Ringbuffer<T>::operator[](size_t i)
{
	if (i >= size_)
	{
		throw std::runtime_error("Ringbuffer indexed out of bounds!");
	}
	return data_[((start_ + i) % max_size_)];
}

} // namespace Anthrax

#endif // RINGBUFFER_HPP
