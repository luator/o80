static int get_bursting(const std::string& segment_id)
{
    int r;
    shared_memory::get<int>(segment_id, "bursting", r);
    return r;
}

static void reset_bursting(const std::string& segment_id)
{
    shared_memory::set<int>(segment_id, "bursting", 0);
}



template<class EXTENDED_STATE>
Spinnable<EXTENDED_STATE>::Spinnable(std::string segment_id,
				     double frequency)
    : segment_id_(segment_id),
      frequency_(frequency),
      period_(static_cast<long int>((1.0 / frequency) * 1E6 + 0.5)),
      frequency_manager_(frequency_),
      now_(time_now()),
      burster_(nullptr)
{}


template<class EXTENDED_STATE>
bool
Spinnable<EXTENDED_STATE>::spin(bool bursting)
{
    if (bursting && burster_ == nullptr)
    {
        burster_ = std::make_shared<Burster>(segment_id_);
    }

    int nb_iterations = 1;
    if (bursting)
    {
        nb_iterations = get_bursting(segment_id_);
        reset_bursting(segment_id_);
    }

    bool should_not_stop = true;

    for (int it = 0; it < nb_iterations; it++)
    {
        // one iteration (reading command, applying them, writing
        // observations to shared memory)

        should_not_stop = iterate(now_);

        // received stop signal from user via shared memory
        if (!should_not_stop)
        {
            break;
        }

        // not in bursting, running at desired frequency
        if (!bursting)
        {
            frequency_manager_.wait();
            now_ = time_now();
        }

        // bursting : running as fast as possible,
        // but keeping track of virtual time
        else
        {
            now_ += period_;
        }
    }

    // bursting : after burst of several iterations,
    // wait for client/python to ask to go again
    if (bursting && should_not_stop)
    {
        burster_->pulse();
    }

    return should_not_stop;

}

template<class EXTENDED_STATE>
void Spinnable<EXTENDED_STATE>::set_extended_state(EXTENDED_STATE& extended_state)
{
    extended_state_=extended_state;
}
