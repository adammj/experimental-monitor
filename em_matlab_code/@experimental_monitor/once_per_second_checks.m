function [status] = once_per_second_checks(obj, varargin)
    
persistent previous_packet_count

current_packet_length = obj.packet_queue.used;
if isempty(previous_packet_count)
    previous_packet_count = current_packet_length;
end

%regular cleanup of delay_call
delay_call('clean');

if ~obj.verify_connection(true)
    return;
end

%default to is connected
status = true;

%if the same as two seconds ago (and more than 0 packets)
%or new count is higher, then ask for avaible space
if ((previous_packet_count == current_packet_length) && (current_packet_length > 0))...
        || (current_packet_length > previous_packet_count)
   
    fprintf('get_event_code_queue_available (%i, %i)\n', previous_packet_count, current_packet_length);
    obj.send_command(obj.commands.get_event_code_queue_available);
end
%store update
previous_packet_count = current_packet_length;


%if the packet queue is empty, and there is a callback, call it
if obj.packet_queue.used == 0
    if ~isempty(obj.packet_queue_is_empty_callback)
        delay_call(0, obj.packet_queue_is_empty_callback);
        obj.packet_queue_is_empty_callback = [];    %clear callback afterwards
    end
    if obj.disconnect_on_empty_queue
        obj.disconnect;
    end
end

try
    %if connected, but the disconnect timer is messed up, reset it
    if obj.is_connected && (~isvalid(obj.auto_disconnect_timer) || ~strcmp(obj.auto_disconnect_timer.Running,'on'))
        obj.check_disconnect_timer('reset');
    %else, if not connected, but the timer is valid, disconnect
    elseif ~obj.is_connected && (isvalid(obj.auto_disconnect_timer) && strcmp(obj.auto_disconnect_timer.Running,'on'))
        fprintf('invalid autodisconnect timer, shutting timers down\n');
        obj.disconnect;
        status = false;
        obj.start_serial_timers(false);
        return;
    end

    %check that other timers are valid
    if isvalid(obj.poll_serial_port_timer) && isvalid(obj.one_per_sec_timer)
        %check that all timers are running
        if strcmp(obj.poll_serial_port_timer.Running,'on') && strcmp(obj.one_per_sec_timer.Running,'on')
        else
            status = false;
        end
    else
        status = false;
    end
catch
    %any errors, just flag as not working
    status = false;
end

%if is connected, but there was an error, 
%then stop and start all timers
if obj.is_connected && obj.connection_verified && ~status 
    fprintf('restarting experimental_monitor polling\n');
    obj.start_serial_timers(false);
    obj.start_serial_timers(true);
end

end
