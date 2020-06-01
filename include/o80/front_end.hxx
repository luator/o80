

#define TEMPLATE_FRONTEND        \
    template <int QUEUE_SIZE,    \
              int NB_ACTUATORS,  \
              class ROBOT_STATE, \
              class EXTENDED_STATE>

#define FRONTEND FrontEnd<QUEUE_SIZE, NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>

namespace internal
{
void set_bursting(const std::string &segment_id, int nb_iterations)
{
    shared_memory::set<int>(segment_id, "bursting", nb_iterations);
}
}

TEMPLATE_FRONTEND
FRONTEND::FrontEnd(std::string segment_id)
    : segment_id_(segment_id),
      commands_(segment_id+"_commands",QUEUE_SIZE,false),
      buffer_commands_(QUEUE_SIZE),
      buffer_index_(0),
      observations_(segment_id+"_observations",QUEUE_SIZE,false),
      completed_commands_(segment_id+"_completed",QUEUE_SIZE,false),
      leader_(nullptr),
      logger_(nullptr)
{
    shared_memory::get<long int>(segment_id_,"pulse_id",
				 pulse_id_);
    pulse_id_++;
    observations_index_ = observations_.newest_timeindex(false);
    internal::set_bursting(segment_id, 1);
}

TEMPLATE_FRONTEND
FRONTEND::~FrontEnd()
{
  if(logger_!=nullptr)
    {
      delete logger_;
    }
}

TEMPLATE_FRONTEND
void
FRONTEND::log(LogAction action)
{
  if(logger_!=nullptr)
    {
      logger_->log(segment_id_,action);
    }
}

TEMPLATE_FRONTEND
void
FRONTEND::start_logging(std::string logger_segment_id)
{
  logger_ = new Logger(QUEUE_SIZE,logger_segment_id,false);
}


TEMPLATE_FRONTEND
int FRONTEND::get_nb_actuators() const
{
    return NB_ACTUATORS;
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>
FRONTEND::wait_for_next()
{
    observations_index_+=1;
    Observation<NB_ACTUATORS,
		ROBOT_STATE,
		EXTENDED_STATE> obs =  observations_[observations_index_];
    return obs;
}

TEMPLATE_FRONTEND
void FRONTEND::reset_next_index()
{
    observations_index_ = observations_.newest_timeindex(false);
}

TEMPLATE_FRONTEND
bool FRONTEND::observations_since(time_series::Index time_index,
				 std::vector<Observation<NB_ACTUATORS,
				 ROBOT_STATE,
				 EXTENDED_STATE>>& v)
{
    time_series::Index oldest = observations_.oldest_timeindex();
    time_series::Index newest = observations_.newest_timeindex();
    if (time_index > newest || time_index<oldest)
	{
	    return false;
	}
    for(time_series::Index index=time_index; index<=newest; index++)
	{
	    v.push_back(observations_[index]);
	}
    return true;
}

TEMPLATE_FRONTEND
std::vector<Observation<NB_ACTUATORS,
			ROBOT_STATE,
			EXTENDED_STATE>>
FRONTEND::get_observations_since(time_series::Index time_index)
{
    std::vector<Observation<NB_ACTUATORS,
			    ROBOT_STATE,
			    EXTENDED_STATE>> v;
    observations_since(time_index,v);
    return v;
}

TEMPLATE_FRONTEND
bool FRONTEND::update_latest_observations(size_t nb_items,
			     std::vector<Observation<NB_ACTUATORS,
			     ROBOT_STATE,
			     EXTENDED_STATE>>& v)
{
    bool r=true;
    time_series::Index oldest = observations_.oldest_timeindex();
    time_series::Index newest = observations_.newest_timeindex();
    time_series::Index target = newest-nb_items+1;
    if (target<oldest)
	{
	    target=oldest;
	    r=false;
	}
    for(time_series::Index index=target; index<=newest; index++)
	{
	    v.push_back(observations_[index]);
	}
    return r;
}

TEMPLATE_FRONTEND
std::vector<Observation<NB_ACTUATORS,
			ROBOT_STATE,
			EXTENDED_STATE>>
FRONTEND::get_latest_observations(size_t nb_items)
{
    std::vector<Observation<NB_ACTUATORS,
			    ROBOT_STATE,
			    EXTENDED_STATE>> v;
    update_latest_observations(nb_items,v);
    return v;
			    
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator,
                           ROBOT_STATE target_state,
                           Iteration target_iteration,
                           Mode mode)
{
  Command<ROBOT_STATE> command(pulse_id_,target_state,target_iteration,
			       nb_actuator,mode);
  buffer_commands_.append(command);
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator,
                           ROBOT_STATE target_state,
                           Speed speed,
                           Mode mode)
{
  Command<ROBOT_STATE> command(pulse_id_,target_state,speed,
			       nb_actuator,mode);
  buffer_commands_.append(command);
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator,
                           ROBOT_STATE target_state,
                           Duration_us duration,
                           Mode mode)
{
  Command<ROBOT_STATE> command(pulse_id_,target_state,duration,
			       nb_actuator,mode);
  buffer_commands_.append(command);
}

TEMPLATE_FRONTEND
void FRONTEND::add_command(int nb_actuator, ROBOT_STATE target_state, Mode mode)
{
  Command<ROBOT_STATE> command(pulse_id_,target_state,
			       nb_actuator,mode);
  buffer_commands_.append(command);
}

TEMPLATE_FRONTEND
time_series::Index FRONTEND::last_index_read_by_backend()
{
  time_series::Index index;
  shared_memory::get<time_series::Index>(segment_id_,
					 "command_read",
					 index);
  return index;
					 
}

TEMPLATE_FRONTEND
void FRONTEND::size_check()
{
    std::size_t nb_new_commands =
	buffer_commands_.newest_timeindex(false)-buffer_index_;
    if(nb_new_commands>buffer_commands_.max_length())
	{
	    throw std::runtime_error("shared memory commands buffer too large");    
	}
    if(nb_new_commands>commands_.max_length())
	{
	    throw std::runtime_error("shared memory commands exchange full");    
	}
    if(commands_.is_empty())
	{
	    return;
	}
    time_series::Index latest_read = last_index_read_by_backend();
    std::size_t nb_not_read_yet = commands_.newest_timeindex(false)-latest_read;
    std::size_t nb_free_slots = commands_.max_length()-nb_not_read_yet;
    if(nb_new_commands>nb_free_slots)
	{
	    throw std::runtime_error("shared memory commands exchange full");    
	}
}


TEMPLATE_FRONTEND
void FRONTEND::share_commands(std::set<int>& command_ids, bool store)
{
    
    // no commands
    if (buffer_commands_.is_empty())
	{
	    return;
	}

    // checking there is no time series overflow
    // (and throwing error if there is)
    size_check();

    // if there is not new commands, exit
    time_series::Index last_index = buffer_commands_.newest_timeindex(false);
    if(last_index<buffer_index_)
	{
	    return;
	}

    // sharing new commands
    for(time_series::Index index=buffer_index_; index<=last_index; index++)
	{
	    Command<ROBOT_STATE> command = buffer_commands_[index];
	    if(store)
		{
		    command_ids.insert(command.get_id());
		}
	    std::cout << "frontend | ";
	    command.print();
	    commands_.append(command);
	}

    // sync with backend
    shared_memory::set<long int>(segment_id_,"pulse_id",pulse_id_);
    pulse_id_++;
    
    buffer_index_ = last_index+1;

}

TEMPLATE_FRONTEND
void FRONTEND::wait_for_completion(std::set<int>& command_ids,
				   time_series::Index completed_index)
{
  completed_index++;
  while(true)
    {
      time_series::Index command_id = completed_commands_[completed_index];
      command_ids.erase(command_id);
      if (command_ids.empty())
	{
	  return;
	}
      completed_index++;
    }
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::pulse(
    Iteration iteration)
{
  share_commands(sent_command_ids_,false);
  observations_.wait_for_timeindex(iteration.value);
  return observations_[iteration.value];
}

TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::pulse()
{
  share_commands(sent_command_ids_,false);
  if(observations_.is_empty())
    {
      return Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>();
    }
  return observations_.newest_element();
}


TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>
FRONTEND::pulse_and_wait()
{
  sent_command_ids_.clear();
  int completed_index=-1;
  if(!completed_commands_.is_empty())
    {
      completed_index = completed_commands_.newest_timeindex();
    }
  share_commands(sent_command_ids_,true);
  wait_for_completion(sent_command_ids_,completed_index);
  return observations_.newest_element();
}


TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::burst(
    int nb_iterations)
{
  share_commands(sent_command_ids_,false);
  internal::set_bursting(segment_id_, nb_iterations);
  if (leader_ == nullptr)
    {
      leader_.reset(
		    new synchronizer::Leader(segment_id_ + "_synchronizer", true));
    }
  leader_->pulse();
  if(observations_.is_empty())
    {
      return Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE>();
    }
  return observations_.newest_element();
}

TEMPLATE_FRONTEND
void FRONTEND::final_burst()
{
    leader_->stop_sync();
}


TEMPLATE_FRONTEND
Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> FRONTEND::read()
{
  Observation<NB_ACTUATORS, ROBOT_STATE, EXTENDED_STATE> observation;
  if(observations_.is_empty())
    {
      return observation;
    }
  observation = observations_.newest_element();
  return observation;
}
