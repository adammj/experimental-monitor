function disconnect(obj, force_disconnect)
%test if the java_serial_object is already empty
if isempty(obj.java_serial_object)
    fprintf('already disconnected\n');
    %do a little cleanup
    obj.close_all_em_instruments;
    obj.delete_all_experimental_monitor_timers;
    return;
end

if nargin < 2
    force_disconnect = false;
end

fprintf('disconnecting... ');

%test if it is still connected
if ~force_disconnect && obj.verify_connection
    %check if there are still event_code_packets to send
    if obj.packet_queue.used > 0
        fprintf('emptying packet queue before disconnecting (max 60 seconds)\n');
        
        %just send a command to make sure it's trying to send
        obj.disconnect_on_empty_queue = true;
        
        delay_call(60, @obj.disconnect, true);
        return;
    end
end

%stop and delete timers
obj.start_serial_timers(false);
obj.check_disconnect_timer('stop');
obj.delete_all_experimental_monitor_timers;

%remove all callbacks
obj.remove_callback(true, true); %remove all callbacks

%clear the configurations
obj.ti_configuration = struct();
obj.db_configuration = struct();
obj.serial_configuration = struct();
obj.connection_verified = false;

%try to close
try
    fclose(obj.serial_connection);
catch
end

%try to delete the serial connection
try
    if strcmp(obj.serial_connection.status, 'closed')
        delete(obj.serial_connection); 
        %clear the local pointers
        obj.serial_connection = [];
    end
catch
end

if ~obj.verify_connection
    fprintf('disconnected\n');
else
    fprintf('error disconnecting\n');
    return;
end

obj.is_connected = false;
obj.java_serial_object = [];

%do a final double check that all instruments with em are closed
obj.close_all_em_instruments;

end
