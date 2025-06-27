## Benchmarking with Hydra

The server includes a benchmark, based on how fast clients can send messages
to eachother.  It does this with simulated clients running inside the server,
which set up a simulated connection to a real server endpoint.  In the first
part, accounts are created, and simulated client connections are established.
This is followed by the actual benchmark.

The default parameters of the benchmark are:

  * 200000 accounts
  * 10000 clients
  * 5000 clients send 10 messages to other, randomly-chosen clients

Before cold booting the server, a change should be made to the configuration of
`cloud-server`, to disable ticks measurement.  The current method of measuring
ticks distorts the benchmark result too much.  In
`cloud-server/src/include/config.h`, change

    # define RSRC_TICKS_MEASUREMENT			/* measure ticks */

into

    # undef RSRC_TICKS_MEASUREMENT			/* measure ticks */
 
To run the benchmark, `telnet` to port 8023, login and execute the following
commands:

    > cd ~MsgServer/benchmark/sys
    > compile benchmark.c
