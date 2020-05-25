

# in this script, in loops of increasing frequencies F:
# - we start a non bursting standalone with frequency F
# - we start a frontend
#     - we samples the observed frequencies of the backend using the front end
#       as the frontend does not send commands
#     - we samples the observed frequencies of the backend using the front end
#       as the frontend send immediates overwriting command
#     - we samples the observed frequencies o the backend using the fron end
#       as the frontend sends iteration target commands (i.e. requires interpolations)
#     - we plot histograms of the above

import o80_example as o80

FREQUENCIES = [50,100,500,1000,2000]
MAX = 1000
MIN = 0
SEGMENT_ID = "o80_frequency_benchmark"
SAMPLE_PERIOD = 0.1
SAMPLE_DURATION = 5
BURSTIN_MODE = False
QUEUE_SIZE = 5000

for frequency in FREQUENCIES:
    
    o80.start_standalone(SEGMENT_ID,
                         frequency,
                         BURSTING_MODE,
                         float("+inf"),
                         float("+inf"),
                         MIN,
                         MAX)

    frontend = o80.FrontEnd(SEGMENT_ID)

    observed_frequencies = [[]]*3
    
    time_start = time.time()
    previous_iteration = None
    while time.time()-time_start < SAMPLE_DURATION:

        observation = frontend.read()
        iteration = observation.get_iteration()
        if iteration!=previous_iteration:
            freq = observation.get_frequency()
            observed_frequencies[0].append(freq)
            time.sleep(SAMPLE_PERIOD)
            previous_iteration = iteration
        else:
            time.sleep(0.01)

    
    time_start = time.time()
    previous_iteration = None
    while time.time()-time_start < SAMPLE_DURATION:

        for dof in range(2):
            for index in range(QUEUE_SIZE):
                value = MIN+float(index)*(MAX-MIN)/float(QUEUE_SIZE)
                frontend.add_command(0)
        observation = frontend.pulse()
        iteration = observation.get_iteration()
        if iteration!=previous_iteration:
            freq = observation.get_frequency()
            observed_frequencies[0].append(freq)
            time.sleep(SAMPLE_PERIOD)
            previous_iteration = iteration
        else:
            time.sleep(0.01)

        
