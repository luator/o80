// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

namespace o80
{
template <int NB_ACTUATORS, class STATE>
ControllersManager<NB_ACTUATORS, STATE>::ControllersManager()
    : completed_commands_(nullptr)
{
    for (int i = 0; i < NB_ACTUATORS; i++)
    {
        initialized_[i] = false;
    }
}

template <int NB_ACTUATORS, class STATE>
void ControllersManager<NB_ACTUATORS, STATE>::set_completed_commands(
    time_series::MultiprocessTimeSeries<CommandId>* completed_commands)
{
    for (Controller<STATE>& controller : controllers_)
    {
        controller.set_completed_commands(completed_commands);
    }
}


  template <int NB_ACTUATORS, class STATE>
bool ControllersManager<NB_ACTUATORS, STATE>::reapplied_desired_states() const
{
    for (int dof = 0; dof < NB_ACTUATORS; dof++)
	{
	    if (!controllers_[dof].reapplied_desired_state())
		{
		    return false;
		}
	}
    return true;
}

template <int NB_ACTUATORS, class STATE>
void ControllersManager<NB_ACTUATORS, STATE>::add_command(
    const Command<STATE>& command)
{
    int dof = command.get_dof();
    if (dof < 0 || dof >= controllers_.size())
    {
        throw std::runtime_error("command with incorrect dof index");
    }
    controllers_[dof].set_command(command);
}

template <int NB_ACTUATORS, class STATE>
STATE ControllersManager<NB_ACTUATORS, STATE>::get_desired_state(
    int dof,
    long int current_iteration,
    const TimePoint& time_now,
    const STATE& current_state)
{
    if (dof < 0 || dof >= controllers_.size())
    {
        throw std::runtime_error("command with incorrect dof index");
    }

    if (!initialized_[dof])
    {
        previous_desired_states_.values[dof] = current_state;
        initialized_[dof] = true;
    }

    const STATE& desired = controllers_[dof].get_desired_state(
        current_iteration,
        current_state,
        previous_desired_states_.values[dof],
        time_now);

    previous_desired_states_.values[dof] = desired;
    return desired;
}

template <int NB_ACTUATORS, class STATE>
int ControllersManager<NB_ACTUATORS, STATE>::get_current_command_id(
    int dof) const
{
    if (dof < 0 || dof >= controllers_.size())
    {
        throw std::runtime_error("command with incorrect dof index");
    }
    return controllers_[dof].get_current_command_id();
}
};
