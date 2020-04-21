#pragma once

#include "o80/observation.hpp"
#include "o80/burster.hpp"
#include "o80/frequency_manager.hpp"

namespace o80
{

    void please_stop(std::string segment_id)
    {
	shared_memory::set<bool>(segment_id, "should_stop", true);
	try
	    {
		synchronizer::Leader leader(segment_id + "_synchronizer");
		leader.pulse();
	    }
	catch (...)
	    {
	    }
    }
    
    template<class EXTENDED_STATE=EmptyExtendedState>
    class Spinnable
    {
    public:
	Spinnable(std::string segment_id,
		  double frequency);
	bool spin(bool bursting = false);
	void set_extended_state(EXTENDED_STATE& extended_state);
    private:
	virtual bool iterate(const TimePoint& time_now);
    protected:
	std::string segment_id_;
	double frequency_;
	Microseconds period_;
	FrequencyManager frequency_manager_;
	TimePoint now_;
	std::shared_ptr<Burster> burster_;
	EXTENDED_STATE extended_state_;
    };

    #include "spinnable.hxx"
    
}
