// Copyright (C) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

template <int QUEUE_SIZE, class STATE>
CommandsSetter<QUEUE_SIZE, STATE>::CommandsSetter(std::string segment_id)

    : segment_id_(segment_id+"_commands"),
      completed_segment_id_(segment_id+"_completed_commands"),
      running_commands_exchange_(segment_id_,QUEUE_SIZE,false),
      completed_commands_exchange_(segment_id_,QUEUE_SIZE,false),
      completed_commands_index_(-1)
{
    // reading from the backend from which command id
    // we should start
    Command<STATE>::init_id(segment_id_, "command_id");
}

template <int QUEUE_SIZE, class STATE>
CommandsSetter<QUEUE_SIZE, STATE>::~CommandsSetter()
{
}

template <int QUEUE_SIZE, class STATE>
int CommandsSetter<QUEUE_SIZE, STATE>::add_command(int dof,
                                                   STATE target_pressure,
                                                   Mode mode)
{
    Command<STATE> command(target_pressure, dof, mode);
    commands_buffer_.push_back(command);
    int id = command.get_id();
    non_completed_commands_.insert(id);
    return id;
}

template <int QUEUE_SIZE, class STATE>
int CommandsSetter<QUEUE_SIZE, STATE>::add_command(int dof,
                                                   STATE target_pressure,
                                                   Iteration iteration,
                                                   Mode mode)
{
    Command<STATE> command(target_pressure, iteration, dof, mode);
    commands_buffer_.push_back(command);
    int id = command.get_id();
    non_completed_commands_.insert(id);
    return id;
}

template <int QUEUE_SIZE, class STATE>
int CommandsSetter<QUEUE_SIZE, STATE>::add_command(int dof,
                                                   STATE target_pressure,
                                                   TimeStamp stamp,
                                                   Mode mode)
{
    Command<STATE> command(target_pressure, stamp, dof, mode);
    commands_buffer_.push_back(command);
    int id = command.get_id();
    non_completed_commands_.insert(id);
    return id;
}

template <int QUEUE_SIZE, class STATE>
int CommandsSetter<QUEUE_SIZE, STATE>::add_command(
    int dof,
    STATE target_pressure,
    std::chrono::microseconds duration,
    Mode mode)
{
    Command<STATE> command(target_pressure, duration, dof, mode);
    commands_buffer_.push_back(command);
    int id = command.get_id();
    non_completed_commands_.insert(id);
    return id;
}

template <int QUEUE_SIZE, class STATE>
int CommandsSetter<QUEUE_SIZE, STATE>::add_command(int dof,
                                                   STATE target_pressure,
                                                   Speed speed,
                                                   Mode mode)
{
    Command<STATE> command(target_pressure, speed, dof, mode);
    commands_buffer_.push_back(command);
    int id = command.get_id();
    non_completed_commands_.insert(id);
    return id;
}

template <int QUEUE_SIZE, class STATE>
void CommandsSetter<QUEUE_SIZE, STATE>::go_to_posture(
    const std::map<int, STATE> &dof_states, int speed, long int time_precision)
{
    std::chrono::microseconds precision(time_precision);
    std::set<int> command_ids;

    for (const std::pair<int, STATE> &dof_state : dof_states)
    {
        int id = this->add_command(
            dof_state.first, dof_state.second, speed, Mode::OVERWRITE);
        command_ids.insert(id);
    }

    this->wait_for_completion(command_ids, precision);
}

template <int QUEUE_SIZE, class STATE>
void CommandsSetter<QUEUE_SIZE, STATE>::wait_for_completion(
    int command_id, std::chrono::microseconds precision)
{
    while (true)
    {
        usleep(precision.count());
        communicate();

        int count = this->non_completed_commands_.count(command_id);
        if (count == 0)
        {
            break;
        }
    }
}

template <int QUEUE_SIZE, class STATE>
bool CommandsSetter<QUEUE_SIZE, STATE>::wait_for_completion(
    std::set<int> &command_ids, std::chrono::microseconds precision)
{
    static std::deque<int> to_remove;
    to_remove.clear();

    bool everything_shared = true;

    while (true && everything_shared)
    {
        usleep(precision.count());
        everything_shared = communicate();

        for (int command_id : command_ids)
        {
            int count = this->non_completed_commands_.count(command_id);
            if (count == 0)
            {
                to_remove.push_back(command_id);
            }
        }
        for (int command_id : to_remove)
        {
            command_ids.erase(command_id);
        }
        to_remove.clear();
        if (command_ids.size() == 0)
        {
            break;
        }
    }

    return everything_shared;
}

template <int QUEUE_SIZE, class STATE>
bool CommandsSetter<QUEUE_SIZE, STATE>::communicate()
{

    // number of available slots in the time series for new commands
    // (we want to avoid commands not yet read by the backend to get
    // lost during rotation of the underlying time series shared memory)
    
    int remaining_size = QUEUE_SIZE - non_completed_commands_.size();

    // sharing all buffered commands with the backend

    // will be set to false if not all buffered commands managed to get
    // shared (because more buffered commands than remaining size)
    bool all_exchanged = true;
    int nb_exchanged = 0;
    while(!commands_buffer_.empty())
	{
	    running_commands_exchange_.append(commands_buffer_.front());
	    // checking we are not sharing more commands than we can,
	    // and if we do, exiting
	    exchanged++;
	    if (exchanged>=remaining_size)
		{
		    all_exchanged=false;
		    breakl
		}
	}

    // updated list of commmands executed by the backend
    time_series::Index newest_index =
	completed_commands_exchange_.newest_timeindex();
    if(completed_commands_index_<0)
	{
	    completed_commands_index_= newest_index+1;
	}
    for(time_series::Index index=completed_commands_index_;
	index<=newest_index;
	index++)
	{
	    int command_id = completed_commands_exchange_[index];
	    non_completed_commands_.erase(id.value);
	}
    completed_commands_index_ = newest_index+1;

    
    // informing user wether of not all buffered commands could be
    // written to shared memory
    return all_exchanged;
}
