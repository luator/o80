// Copyright (C) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

template <int QUEUE_SIZE, class STATE>
CommandsSetter<QUEUE_SIZE, STATE>::CommandsSetter(std::string segment_id)

    : segment_id_(segment_id + "_commands"),
      completed_segment_id_(segment_id + "_completed_commands"),
      running_commands_exchange_(segment_id_, QUEUE_SIZE, false),
      completed_commands_exchange_(completed_segment_id_, QUEUE_SIZE, false)
{
    // reading from the backend from which command id
    // we should start
    Command<STATE>::init_id(segment_id, "command_id");
    completed_commands_exchange_index_ =
        completed_commands_exchange_.newest_timeindex(false) + 1;
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
    const std::map<int, STATE>& dof_states, int speed, long int time_precision)
{
    std::chrono::microseconds precision(time_precision);
    std::set<int> command_ids;

    for (const std::pair<int, STATE>& dof_state : dof_states)
    {
        int id = this->add_command(
            dof_state.first, dof_state.second, speed, Mode::OVERWRITE);
        command_ids.insert(id);
    }

    this->wait_for_completion(command_ids, precision);
}

template <int QUEUE_SIZE, class STATE>
void CommandsSetter<QUEUE_SIZE, STATE>::wait_for_completion(
    int command_id,
    std::chrono::microseconds precision,
    bool& everything_shared,
    bool& too_many_not_completed)
{
    while (true)
    {
        usleep(precision.count());
        communicate(everything_shared, too_many_not_completed);

        int count = this->non_completed_commands_.count(command_id);
        if (count == 0)
        {
            break;
        }
    }
}

template <int QUEUE_SIZE, class STATE>
void CommandsSetter<QUEUE_SIZE, STATE>::wait_for_completion(
    std::set<int>& command_ids,
    std::chrono::microseconds precision,
    bool& everything_shared,
    bool& too_many_not_completed)
{
    static std::deque<int> to_remove;
    to_remove.clear();

    everything_shared = true;
    too_many_not_completed = false;

    while (true && everything_shared && !too_many_not_completed)
    {
        usleep(precision.count());
        communicate(everything_shared, too_many_not_completed);

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
}

template <int QUEUE_SIZE, class STATE>
void CommandsSetter<QUEUE_SIZE, STATE>::communicate(
    bool& all_exchanged, bool& too_many_not_completed)
{
    // will be set to false if not all buffered commands managed to get
    // shared (because more buffered commands than remaining size)
    all_exchanged = true;

    // will be set to true if there are too many not completed commands
    // (comparatively to the size of the shared memory)
    too_many_not_completed = false;

    // number of available slots in the time series for new commands
    // (we want to avoid commands not yet read by the backend to get
    // lost during rotation of the underlying time series shared memory)

    int remaining_size = QUEUE_SIZE - non_completed_commands_.size();

    // sharing all buffered commands with the backend
    int nb_exchanged = 0;
    while (!commands_buffer_.empty())
    {
        running_commands_exchange_.append(commands_buffer_.front());
        // checking we are not sharing more commands than we can,
        // and if we do, exiting
        nb_exchanged++;
        commands_buffer_.pop_front();
        if (nb_exchanged >= remaining_size)
        {
            all_exchanged = false;
            break;
        }
    }

    // updated list of commmands executed by the backend
    time_series::Index newest_index =
        completed_commands_exchange_.newest_timeindex(false);
    if (newest_index >= 0)
    {
        for (time_series::Index index = completed_commands_exchange_index_;
             index <= newest_index;
             index++)
        {
            CommandId command_id = completed_commands_exchange_[index];
            non_completed_commands_.erase(command_id.value);
        }
        completed_commands_exchange_index_ = newest_index + 1;
    }

    // checking if there are not too many non completed commands
    // compared to the available shared memory size
    int nb_non_completed = non_completed_commands_.size();
    if (nb_non_completed >= QUEUE_SIZE)
    {
        too_many_not_completed = true;
    }
}
