// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#define TEMPLATE_BACKEND        \
    template <int QUEUE_SIZE,   \
              int NB_ACTUATORS, \
              class STATE,      \
              class EXTENDED_STATE>

#define BACKEND BackEnd<QUEUE_SIZE, NB_ACTUATORS, STATE, EXTENDED_STATE>

void clear_shared_memory(std::string segment_id)
{
    shared_memory::clear_shared_memory(segment_id);
    shared_memory::clear_shared_memory(segment_id + std::string("_commands"));
    shared_memory::clear_shared_memory(segment_id +
                                       std::string("_completed_commands"));
    shared_memory::clear_shared_memory(segment_id +
                                       std::string("_synchronizer"));
    shared_memory::clear_shared_memory(segment_id +
                                       std::string("_synchronizer_follower"));
    shared_memory::clear_shared_memory(segment_id +
                                       std::string("_synchronizer_leader"));
    // mutex cleaned on destruction
    shared_memory::Mutex m1(segment_id + std::string("_locker"), true);
    shared_memory::Mutex m2(
        std::string("completed_") + segment_id + std::string("_locker"), true);
}

TEMPLATE_BACKEND
BACKEND::BackEnd(std::string segment_id)
    : segment_id_(segment_id),
      commands_getter_(segment_id + "_commands", QUEUE_SIZE, true),
      commands_getter_index_(0),
      completed_commands_(segment_id + "_completed_commands", QUEUE_SIZE, true),
      observation_exchange_(
          segment_id, std::string("observations"), QUEUE_SIZE, true),
      desired_states_(),
      iteration_(0),
      observed_frequency_(0.0)
{
    frequency_measure_.tick();
    controllers_manager_.set_completed_commands(&completed_commands_);
    // initializing the command id to zero. It is written
    // (and later on updated) in the shared memory
    // so that new front end will know from which command id they should start.
    Command<STATE>::set_id(segment_id, "command_id", 0);
}

TEMPLATE_BACKEND
BACKEND::~BackEnd()
{
    clear_shared_memory(segment_id_);
}

TEMPLATE_BACKEND
void BACKEND::iterate(const TimePoint& time_now,
                      const States<NB_ACTUATORS, STATE>& current_states,
                      bool iteration_update,
                      long int current_iteration)
{
    if (iteration_update)
    {
        iteration_++;
    }
    else
    {
        iteration_ = current_iteration;
    }

    // reading new commands and dispatching them to the
    // controllers manager
    if (commands_getter_.length() > 0)
    {
        time_series::Index newest_index = commands_getter_.newest_timeindex();
        if (newest_index >= 0)
        {
            for (time_series::Index index = commands_getter_index_;
                 index <= newest_index;
                 index++)
            {
                controllers_manager_.add_command(commands_getter_[index]);
            }
            commands_getter_index_ = newest_index + 1;
            // keeping track of latest command id, to share with new frontends
            Command<STATE>::set_id(segment_id_,
                                   "command_id",
                                   commands_getter_[newest_index].get_id());
        }
    }

    // reading desired state based on controllers output
    for (int controller_nb = 0; controller_nb < desired_states_.values.size();
         controller_nb++)
    {
        desired_states_.values[controller_nb] =
            controllers_manager_.get_desired_state(
                controller_nb,
                iteration_,
                time_now,
                current_states.values[controller_nb]);
    }

    // note : executed commands id are written to
    // the shared time series completed_commands_ directly by controllers

    observed_frequency_ = frequency_measure_.tick();
}

TEMPLATE_BACKEND
const States<NB_ACTUATORS, STATE>& BACKEND::pulse(
    const TimePoint& time_now,
    const States<NB_ACTUATORS, STATE>& current_states,
    const EXTENDED_STATE& extended_state,
    bool iteration_update,
    long int current_iteration,
    bool print_observation)
{
    iterate(time_now, current_states, iteration_update, current_iteration);

    // writting current states to shared memory
    if (print_observation)
    {
        Observation<NB_ACTUATORS, STATE, EXTENDED_STATE> observation(
            current_states,
            desired_states_,
            extended_state,
            time_now.count(),
            iteration_,
            observed_frequency_);
        observation_exchange_.write(observation);
    }

    return desired_states_;
}

TEMPLATE_BACKEND
const States<NB_ACTUATORS, STATE>& BACKEND::pulse(
    const TimePoint& time_now,
    const States<NB_ACTUATORS, STATE>& current_states,
    bool iteration_update,
    long int current_iteration,
    bool print_observation)
{
    iterate(time_now, current_states, iteration_update, current_iteration);

    // writting current states to shared memory
    if (print_observation)
    {
        Observation<NB_ACTUATORS, STATE, EXTENDED_STATE> observation(
            current_states,
            desired_states_,
            time_now.count(),
            iteration_,
            observed_frequency_);
        observation_exchange_.write(observation);
    }

    return desired_states_;
}
