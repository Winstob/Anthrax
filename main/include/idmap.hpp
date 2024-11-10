/* ---------------------------------------------------------------- *\
 * idmap.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-27
\* ---------------------------------------------------------------- */
#ifndef IDMAP_HPP
#define IDMAP_HPP

#include <vector>
#include <queue>

namespace Anthrax
{

template <class T>
class IDMap
{
public:
	IDMap<T>() {}
	unsigned int add(T value);
	int remove(unsigned int id);
	T& operator[](unsigned int i);
	bool exists(unsigned int i);
	int size() { return data_.size(); }

private:
	std::vector<T> data_;
	unsigned int next_free_id_counter_ = 0;
	std::priority_queue<unsigned int> next_free_id_queue_;
	std::vector<bool> valid_indices_;
};


template <class T>
unsigned int IDMap<T>::add(T value)
{
	unsigned int id;
	if (next_free_id_queue_.size() == 0)
	{
		id = next_free_id_counter_;
		next_free_id_counter_++;
		data_.push_back(value);
		valid_indices_.push_back(true);
	}
	else
	{
		id = next_free_id_queue_.top();
		next_free_id_queue_.pop();
		data_[id] = value;
		valid_indices_[id] = true;
	}
	return id;
}


template <class T>
int IDMap<T>::remove(unsigned int id)
{
	if (id >= next_free_id_counter_ || !valid_indices_[id])
		return 0;

	valid_indices_[id] = false;
	next_free_id_queue_.push(id);
	return 1;
}


template <class T>
T& IDMap<T>::operator[](unsigned int i)
{
	return data_[i];
}

template <class T>
bool IDMap<T>::exists(unsigned int i)
{
	return i < next_free_id_counter_ && valid_indices_[i];
}

} // namespace Anthrax

#endif // IDMAP_HPP


