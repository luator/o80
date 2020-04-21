// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once

namespace o80
{

    template<class DATA>
    class SensorDriver
    {
    public:
	virtual const DATA& get_data()=0;
	virtual void start()=0;
	virtual void stop()=0;
    };
    
}
