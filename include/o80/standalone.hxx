// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#define TEMPLATE_STANDALONE         \
    template <int QUEUE_SIZE,       \
              int NB_ACTUATORS,     \
              class RI_ACTION,      \
              class RI_OBSERVATION, \
              class o80_STATE,      \
              class o80_EXTENDED_STATE>

#define STANDALONE             \
    Standalone<QUEUE_SIZE,     \
               NB_ACTUATORS,   \
               RI_ACTION,      \
               RI_OBSERVATION, \
               o80_STATE,      \
               o80_EXTENDED_STATE>

TEMPLATE_STANDALONE
STANDALONE::Standalone(RiDriverPtr ri_driver_ptr,
                       double frequency,
                       std::string segment_id)
    : Spinnable<o80_EXTENDED_STATE>(segment_id,frequency),
    segment_id_(segment_id),
    ri_driver_ptr_(ri_driver_ptr),
    ri_data_ptr_(std::make_shared<RiData>()),
    ri_frontend_(ri_data_ptr_),
    o8o_backend_(segment_id),
    ri_backend_ptr_(nullptr)
{
    std::cout << "\n\nSTANDALONE: should fix segment ids ! same for burster and backend ???\n\n";
    shared_memory::set<bool>(segment_id, "should_stop", false);
}

TEMPLATE_STANDALONE
STANDALONE::~Standalone()
{
    stop();
}

TEMPLATE_STANDALONE
void STANDALONE::start()
{
    if (ri_backend_ptr_ == nullptr)
    {
        ri_backend_ptr_ =
            new RiBackend(ri_driver_ptr_,
                          ri_data_ptr_,
                          std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity());
        ri_backend_ptr_->initialize();

        RI_ACTION action;
        ri_frontend_.append_desired_action(action);

        std::cout
            << "WARNING: o80 standalone start: setting random first action !\n";
    }
    else
    {
        throw std::runtime_error("a standalone should not be started twice");
    }
    // spinner_.set_frequency(frequency_);
}

TEMPLATE_STANDALONE
void STANDALONE::stop()
{
    if (ri_backend_ptr_ != nullptr)
    {
        delete ri_backend_ptr_;
        ri_backend_ptr_ = nullptr;
    }
}

TEMPLATE_STANDALONE
bool STANDALONE::iterate(const TimePoint& time_now)
{
    // reading sensory info from the robot (robot_interfaces)

    robot_interfaces::TimeIndex time_index =
        ri_frontend_.get_current_timeindex();

    RI_OBSERVATION ri_current_states = ri_frontend_.get_observation(time_index);

    // converting robot_interfaces sensory reading to o80 state
    o80::States<NB_ACTUATORS, o80_STATE> o8o_current_states =
        convert(ri_current_states);

    // adding information to extended state, based on all what is available
    enrich_extended_state(this->extended_state_, ri_current_states);

    // o80 machinery : reading the stack of command and using controller to
    // compute
    //                  desired state for each actuator, writing observation to
    //                  shared memory
    const o80::States<NB_ACTUATORS, o80_STATE>& desired_states =
        o8o_backend_.pulse(time_now, o8o_current_states, this->extended_state_);

    // converting o80 desired state to action to input to robot interface
    RI_ACTION action = convert(desired_states);

    // applying actions to robot
    robot_interfaces::TimeIndex ti = ri_frontend_.append_desired_action(action);

    // check if stop command written by user in shared memory
    bool should_stop;
    shared_memory::get<bool>(segment_id_, "should_stop", should_stop);

    return !should_stop;
}

template <class RobotDriver, class o80Standalone, typename... Args>
void start_action_timed_standalone(std::string segment_id,
                                   double frequency,
                                   bool bursting,
                                   Args&&... args)
{
    if (internal::standalone_exists(segment_id))
    {
        std::string error = std::string("standalone ");
        error += segment_id;
        error += std::string(" already exists");
        throw std::runtime_error(error);
    }

    o80::clear_shared_memory(segment_id);

    typedef internal::StandaloneRunner<RobotDriver, o80Standalone> SR;
    typedef std::shared_ptr<SR> SRPtr;

    SRPtr runner(
        new SR(segment_id, frequency, bursting, std::forward<Args>(args)...));
    runner->start();
    internal::add_standalone(segment_id, runner);
}

template <class RobotDriver, class o80Standalone, typename... Args>
void start_standalone(std::string segment_id,
                      double frequency,
                      bool bursting,
                      Args&&... args)
{
    start_action_timed_standalone<RobotDriver, o80Standalone, Args...>(
        segment_id, frequency, bursting, std::forward<Args>(args)...);
}

bool standalone_is_running(std::string segment_id)
{
    if (!internal::standalone_exists(segment_id))
    {
        return false;
    }
    internal::StandalonePtr& runner = internal::get_standalone(segment_id);
    return runner->is_running();
}

void stop_standalone(std::string segment_id)
{
    if (!internal::standalone_exists(segment_id))
    {
        std::string error = std::string("standalone ");
        error += segment_id;
        error += std::string(" does not exist");
        throw std::runtime_error(error);
    }

    internal::StandalonePtr& runner = internal::get_standalone(segment_id);

    runner->stop();

    o80::clear_shared_memory(segment_id);
}
