// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once

#include "shared_memory/shared_memory.hpp"
#include "time_series/multiprocess_time_series.hpp"

#include "o80/sensor_data.hpp"
#include "o80/frequency_measure.hpp"

namespace o80
{

    template<int QUEUE_SIZE, class DATA>
    class SensorBackEnd
    {

    public:
	typedef time_series::MultiprocessTimeSeries<
	    SensorData<Data> > History;
	
    public:

	SensorBackEnd(std::string segment_id,
	void iterate(const TimePoint& time_stamp,
		     const Data& data);
	
    private:
	FrequencyMeasure frequency_measure_;		
	std::string segment_id_;
	History history_;
    };

#include "sensor_back_end.hxx"
	
}
