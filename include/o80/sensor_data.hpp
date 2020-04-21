// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once
#include "shared_memory/serializer.hpp"

namespace 080
{

    template<class DATA>
    class SensorData
    {

    public:
	SensorData();
	SensorData(long int time_stamp,
		   long int iteration,
		   double frequency,
		   Data data);
	long int get_time_stamp() const;
	long int get_iteration() const;
	double get_frequency() const;
	const Data& get_data() const;
	
    public:
	template <class Archive>
	void serialize(Archive& archive)
	{
	    archive(time_stamp_,
		    iteration_,
		    frequency_,
		    data_);
	}

    private:
	friend shared_memory::private_serialization;	
	long int time_stamp_;
	long int iteration_;
	double frequency_;
	Data data_;
    };

}
