function start_serial_timers(obj, start_yes)

if (start_yes)
    %create the poll timer
    poll_period = round(1/obj.poll_rate, 3);
    poll_period = max(poll_period, 0.001);    %minimum of 0.001 required
    try
        delete(obj.poll_serial_port_timer);
    catch
    end
    obj.poll_serial_port_timer = timer('TimerFcn', @obj.poll_serial_port, 'Name', 'poll_serial_port_timer', 'BusyMode', 'drop', 'ExecutionMode', 'fixedRate', 'Period', poll_period, 'UserData', 'experimental_monitor'); 

    %create the timer-checking timer (1 Hz)
    try
        delete(obj.one_per_sec_timer);
    catch
    end
    obj.one_per_sec_timer = timer('TimerFcn', @obj.once_per_second_checks, 'Name', 'once_per_sec_timer', 'BusyMode', 'drop', 'ExecutionMode', 'fixedRate', 'Period', 1, 'UserData', 'experimental_monitor');

    %start the timers
    start(obj.poll_serial_port_timer);
    start(obj.one_per_sec_timer);
else
    %stop the timers
    try
        stop(obj.one_per_sec_timer);
        delete(obj.one_per_sec_timer);
    catch
    end

    try
        stop(obj.poll_serial_port_timer);
        delete(obj.poll_serial_port_timer);
    catch
    end
end

end
