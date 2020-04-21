
template<int QUEUE_SIZE,
	 class DATA,
	 class DRIVER>
SensorStandalone<QUEUE_SIZE,DATA,DRIVER>::SensorStandalone(DriverPtr driver_ptr,
							   double frequency,
							   std::string segment_id)
    : Spinnable(frequency),
      segment_id_(segment_id),
      driver_ptr_(driver_ptr),
      backend_(segment_id)
{
    shared_memory::set<bool>(segment_id, "should_stop", false);
}

template<int QUEUE_SIZE,
	 class DATA,
	 class DRIVER>
~SensorStandalone<QUEUE_SIZE,DATA,DRIVER>::SensorStandalone()
{
    stop();
}

template<int QUEUE_SIZE,
	 class DATA,
	 class DRIVER>
void 
SensorStandalone<QUEUE_SIZE,DATA,DRIVER>::start()
{
    driver_ptr->start();
}


template<int QUEUE_SIZE,
	 class DATA,
	 class DRIVER>
void
SensorStandalone<QUEUE_SIZE,DATA,DRIVER>::stop()
{
    driver_ptr->stop();
}

template<int QUEUE_SIZE,
	 class DATA,
	 class DRIVER>
bool
SensorStandalone<QUEUE_SIZE,DATA,DRIVER>::iterate(const TimePoint& time_now)
{

    const DATA& data = driver_ptr->get_data();
    backend_.iterate(time_now,data);
    
    // check if stop command written by user in shared memory
    bool should_stop;
    shared_memory::get<bool>(segment_id_, "should_stop", should_stop);

    return !should_stop;
}


