

#define TEMPLATE_FRONTEND        \
    template <int QUEUE_SIZE,    \
              int NB_ACTUATORS,  \
              class ROBOT_STATE, \
              class EXTENDED_STATE>

#define FRONTEND FrontEnd<QUEUE_SIZE, NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>

namespace internal
{
void set_bursting(const std::string& segment_id, int nb_iterations)
{
    shared_memory::set<int>(segment_id, "bursting", nb_iterations);
}
}

TEMPLATE_FRONTEND
FRONTEND::FrontEnd(std::string segment_id)
    : segment_id_(segment_id),
      observation_exchange_(
          segment_id, std::string("observations"), QUEUE_SIZE, false),
      commands_setter_(segment_id),
      leader_(nullptr)
{
    internal::set_bursting(segment_id, 1);
}

TEMPLATE_FRONTEND
time_series::Index FRONTEND::get_current_iteration()
{
    return observation_exchange_.get_history().newest_timeindex();
}

TEMPLATE_FRONTEND
int FRONTEND::get_nb_actuators() const
{
    return NB_ACTUATORS;
}

TEMPLATE_FRONTEND
bool FRONTEND::update_history_since(
    time_series::Index time_index,
    std::vector<Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>>& v)
{
    History& history = observation_exchange_.get_history();
    time_series::Index oldest = history.oldest_timeindex();
    time_series::Index newest = history.newest_timeindex();
    if (time_index > newest || time_index < oldest)
    {
        return false;
    }
    for (time_series::Index index = time_index; index <= newest; index++)
    {
        v.push_back(history[index]);
    }
    return true;
}

TEMPLATE_FRONTEND
std::vector<Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>>
FRONTEND::get_history_since(time_series::Index time_index)
{
    std::vector<Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>> v;
    update_history_since(time_index, v);
    return v;
}

TEMPLATE_FRONTEND
bool FRONTEND::update_latest(
    size_t nb_items,
    std::vector<Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>>& v)
{
    bool r = true;
    History& history = observation_exchange_.get_history();
    time_series::Index oldest = history.oldest_timeindex();
    time_series::Index newest = history.newest_timeindex();
    time_series::Index target = newest - nb_items + 1;
    if (target < oldest)
    {
        target = oldest;
        r = false;
    }
    for (time_series::Index index = target; index <= newest; index++)
    {
        v.push_back(history[index]);
    }
    return r;
}

TEMPLATE_FRONTEND
std::vector<Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>>
FRONTEND::get_latest(size_t nb_items)
{
    std::vector<Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>> v;
    update_latest(nb_items, v);
    return v;
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator,
                           ROBOT_STATE target_state,
                           Iteration target_iteration,
                           Mode mode)
{
    int command_id = commands_setter_.add_command(
        nb_actuator, target_state, target_iteration, mode);
    command_ids_.insert(command_id);
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator,
                           ROBOT_STATE target_state,
                           Speed speed,
                           Mode mode)
{
    int command_id =
        commands_setter_.add_command(nb_actuator, target_state, speed, mode);
    command_ids_.insert(command_id);
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator, ROBOT_STATE target_state, Mode mode)
{
    int command_id =
        commands_setter_.add_command(nb_actuator, target_state, mode);
    command_ids_.insert(command_id);
}

TEMPLATE_FRONTEND
void FRONTEND::communicate()
{
    bool everything_shared;
    bool too_many_not_completed;
    commands_setter_.communicate(everything_shared, too_many_not_completed);
    if (!everything_shared)
    {
        throw std::runtime_error("shared memory for commands exchange full");
    }
    if (too_many_not_completed)
    {
        throw std::runtime_error(
            "shared memory for not-completed commands exchange full");
    }
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::pulse(
    Iteration iteration)
{
    command_ids_.clear();
    communicate();

    Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> observation;
    while (true)
    {
        observation_exchange_.read(observation);
        long int current_iteration = observation.get_iteration();
        if (current_iteration < iteration.value)
        {
            usleep(5);
        }
        else
        {
            return observation;
        }
    }
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::pulse()
{
    command_ids_.clear();
    communicate();
    Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> observation;
    observation_exchange_.read(observation);
    return observation;
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::burst(
    int nb_iterations)
{
    command_ids_.clear();
    communicate();
    internal::set_bursting(segment_id_, nb_iterations);
    if (leader_ == nullptr)
    {
        leader_.reset(
            new synchronizer::Leader(segment_id_ + "_synchronizer", true));
    }
    leader_->pulse();
    Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> observation;
    observation_exchange_.read(observation);
    return observation;
}

TEMPLATE_FRONTEND
void FRONTEND::final_burst()
{
    leader_->stop_sync();
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>
FRONTEND::pulse_and_wait()
{
    bool everything_shared;
    bool too_many_not_completed;
    commands_setter_.wait_for_completion(command_ids_,
                                         o80::Microseconds(10),
                                         everything_shared,
                                         too_many_not_completed);
    if (!everything_shared)
    {
        throw std::runtime_error("shared memory for commands exchange full");
    }
    if (too_many_not_completed)
    {
        throw std::runtime_error(
            "shared memory for not-completed commands exchange full");
    }
    command_ids_.clear();
    Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> observation;
    observation_exchange_.read(observation);
    return observation;
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::read()
{
    Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> observation;
    observation_exchange_.read(observation);
    return observation;
}
