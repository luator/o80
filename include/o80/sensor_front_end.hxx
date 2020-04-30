template<int QUEUE_SIZE,class DATA>
SensorFrontEnd<QUEUE_SIZE,DATA>::SensorFrontEnd(std::string segment_id)
    : segment_id_(segment_id),
      history_(segment_id,
	       QUEUE_SIZE,false)
{
    history_index_ = history_.newest_timeindex(false);
}


template<int QUEUE_SIZE,class DATA>
time_series::Index
SensorFrontEnd<QUEUE_SIZE,DATA>::get_current_iteration()
{
    return history_.newest_timeindex();
}

template<int QUEUE_SIZE,class DATA>
SensorData<DATA>
SensorFrontEnd<QUEUE_SIZE,DATA>::FRONTEND::wait_for_next()
{
    history_index_+=1;
    return history_[history_index_];
}

template<int QUEUE_SIZE,class DATA>
bool
SensorFrontEnd<QUEUE_SIZE,DATA>::::update_history_since(
    time_series::Index time_index,
    std::vector<SensorData<DATA>>& v)
{
    time_series::Index oldest = history_.oldest_timeindex();
    time_series::Index newest = history_.newest_timeindex();
    if (time_index > newest || time_index < oldest)
    {
        return false;
    }
    for (time_series::Index index = time_index; index <= newest; index++)
    {
        v.push_back(history_[index]);
    }
    return true;
}

template<int QUEUE_SIZE,class DATA>
std::vector<SensorData<DATA>>
SensorFrontEnd<QUEUE_SIZE,DATA>::get_history_since(time_series::Index time_index)
{
    std::vector<SensorData<DATA>> v;
    update_history_since(time_index, v);
    return v;
}

template<int QUEUE_SIZE,class DATA>
bool
SensorFrontEnd<QUEUE_SIZE,DATA>::update_latest(
    size_t nb_items,
    std::vector<SensorData<DATA>>& v)
{
    bool r = true;
    time_series::Index oldest = history_.oldest_timeindex();
    time_series::Index newest = history_.newest_timeindex();
    time_series::Index target = newest - nb_items + 1;
    if (target < oldest)
    {
        target = oldest;
        r = false;
    }
    for (time_series::Index index = target; index <= newest; index++)
    {
        v.push_back(history_[index]);
    }
    return r;
}

template<int QUEUE_SIZE,class DATA>
std::vector<SensorData<DATA>>
SensorFrontEnd<QUEUE_SIZE,DATA>::get_latest(size_t nb_items)
{
    std::vector<SensorData<DATA>> v;
    update_latest(nb_items, v);
    return v;
}
