// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once
#include "shared_memory/shared_memory.hpp"
#include "time_series/multiprocess_time_series.hpp"
#include "o80/sensor_data.hpp"

namespace o80
{

    template<int QUEUE_SIZE,class DATA>
    class SensorFrontEnd
    {
	
    public:
	typedef time_series::MultiprocessTimeSeries<
	SensorData<Data> > History;
	typedef std::vector<SensorData<DATA>> HistoryChunk;
	
    public:
	SensorFrontEnd(std::string segment_id);
	time_series::Index get_current_iteration();
	bool update_history_since(time_series::Index iteration,
				  HistoryChunk& push_back_to);
	bool update_latest(size_t nb_items, HistoryChunk& push_back_to);
	
	HistoryChunk get_history_since(time_series::Index iteration);
	HistoryChunk get_latest(size_t nb_items);
	DATA wait_for_next();

    private:
	std::string segment_id_;
	History history_;
	time_series::Index history_index_;

#include "sensor_front_end.hxx"

    };
    
}
