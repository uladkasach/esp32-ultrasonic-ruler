This package does two things:
1. reads data with the ESP-32 (the hard part)
2. receives data with the server (super easy)

### To load the program onto your board and run it you must:
1. setup the environment
    - part 1: setup espressif framework
        - see the instructions in the file `_dev/setup_and_hello_world.sh`
    - part 2: setup the arduino component for espressif
        - run `./init.sh`
            - run `chmod +x ./init.sh` if you need to enable permission to execute the file
3. configure the settings
    - 1. find the LAN-IP of your computer and set it as the `RECEIVER_IP_ADDR` in `main.c`
    - 2. play with the `CONSUMER_DELAY_MILLISECOND`, `PRODUCER_DELAY_MILLISECOND`, `DATA_PER_OUTPUT`, and `QUEUE_SIZE` to tune them to your liking
2. run `./build --build --flash --monitor`
    - `--build` compiles the code
    - `--flash` ports it to the board
    - `--monitor` restarts the board and displays the output of the board
        - e.g., logs; it shows you the serial port

TODO:
- make udp server log to csv file so that it can be used for statistical analysis
- calibrate the "distance" calculated by the sensor
    - it should be pretty close but needs verified.

NOTE:
sometimes the packtets that are sent are corrupt; make sure you get rid of those
