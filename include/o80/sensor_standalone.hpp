// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once

#include "o80/sensor_backend.hpp"
#include "o80/sensor_data.hpp"
#include "o80/spinnable.hpp"
#include "o80/frequency_measure.hpp"
#include "o80/frequency_manager.hpp"

namespace o80
{

    template<int QUEUE_SIZE,
	     class DATA,
	     class DRIVER>
    class SensorStandalone : public Spinnable
    {

    private:
	typedef std::shared_ptr<DRIVER> DriverPtr;
	typedef SensorBackend<QUEUE_SIZE,DATA> Backend; 
	
    public:
	static constexpr int queue_size = QUEUE_SIZE;

	SensorStandalone(DriverPtr driver_ptr,
			 double frequency,
			 std::string segment_id);

	~SensorStandalone();

	void start();
	void stop();
	bool spin(bool bursting=false);

    private:

	bool iterate(const TimePoint& time_now);

    private:
	std::string segment_id_;
	DriverPtr driver_ptr_;
	Backend backend_;
	    
    };

    #include "sensor_standalone.hxx"
}
