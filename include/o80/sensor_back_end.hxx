
template<int QUEUE_SIZE, class DATA>
SensorBackEnd<QUEUE_SIZE,DATA>::SensorBackEnd(std::string segment_id)
    : segment_id_(segment_id),
      history_(segment_id,
	       QUEUE_SIZE,
	       true)
{}

template<int QUEUE_SIZE, class DATA>
void
SensorBackEnd<QUEUE_SIZE,DATA>::iterate(const TimePoint& time_stamp,
					const Data& data)
{
    iteration_++;
    double frequency = frequency_measure.tick();
    SensorData d(time_stamp,
		 iteration_,
		 frequency,
		 data);
    history_.append(d);
}
