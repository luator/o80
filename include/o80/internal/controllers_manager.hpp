// Copyright (C) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once

#include <memory>
#include "command.hpp"
#include "controller.hpp"
#include "o80/states.hpp"

namespace o80
{
template <int NB_ACTUATORS, class STATE>
class ControllersManager
{
private:
    typedef std::array<Controller<STATE>, NB_ACTUATORS> Controllers;

public:
    ControllersManager();

    void set_executed_commands(time_series::MultiprocessTimeSeries<Command<STATE>>*
			       executed_commands);
    
    void add_command(const Command<STATE> &command);

    STATE get_desired_state(int dof,
                            long int current_iteration,
                            const TimePoint &time_now,
                            const STATE &current_state);

    int get_current_command_id(int dof) const;

private:
    time_series::MultiprocessTimeSeries<Command<STATE>>* executed_commands_;
    Controllers controllers_;
    States<NB_ACTUATORS, STATE> previous_desired_states_;
    std::array<bool, NB_ACTUATORS> initialized_;
};
}

#include "controllers_manager.hxx"
