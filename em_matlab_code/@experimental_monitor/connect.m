function connect(obj, serial_port)

if nargin < 2
    serial_port = '';
end

if obj.verify_connection
    fprintf('already connected\n');
    return;
else
    %first make sure it is closed and erased
    obj.close_all_em_instruments;
    
    %clear out everything
    obj.is_connected = false;
    obj.rx_char_buffer = '';
    obj.receive_buffer.clear;
    obj.error_buffer.clear;
    obj.raw_rx_buffer.clear;
    obj.packet_queue.clear;
    obj.connection_verified = false;
    obj.disconnect_on_empty_queue = false;
    delay_call('reset');
    
    %delete all em timers
    obj.delete_all_experimental_monitor_timers;
    
    %clear the serial connection (never use the previous)
    delete(obj.serial_connection);
    obj.serial_connection = [];
    obj.java_serial_object = [];
    
    success = search_for_monitor(obj, serial_port);
    
    %if it found a monitor, now verify it
    if success
        obj.is_connected = true;
       
        fprintf('update configuration (up to 5 sec)...\n');
        finish_making_connection;

        %wait for the connection to be verified, or 5 seconds
        start_tic = tic;
        while ~obj.connection_verified && toc(start_tic) < 5
            pause(0.05); drawnow;  %flush java events
        end

        %if it wasn't verified, then disconnect
        if ~obj.connection_verified
            fprintf(2, 'connection was not verified\n');
            obj.disconnect;
        end
    end
end


%below here are the finish connection steps
    function finish_making_connection()
        %there are pauses in here, which are necessary because this
        %function must be called syncronously
        
        obj.java_serial_object = igetfield(obj.serial_connection, 'jobject');
        obj.start_serial_timers(true);

        %ask for command lists
        obj.update_command_list;
    
        %wait to up to 2 seconds, or command lists
        start_time = tic;
        while (toc(start_time) < 2) && (length(fieldnames(obj.commands)) < 2)
            pause(0.05); drawnow;  %flush java events
        end
    
        if length(fieldnames(obj.commands)) < 2
            fprintf('matlab error: no commands received\n')
            obj.disconnect;
            return;
        end
        
        %ask for serial configuration
        obj.update_serial_configuration;
        
        %wait to up to 2 seconds, or serial configuration
        start_time = tic;
        while (toc(start_time) < 2) && (isempty(fieldnames(obj.serial_configuration)))
            pause(0.05); drawnow;  %flush java events
        end
        
        if isempty(fieldnames(obj.serial_configuration))
            fprintf('matlab error: serial_configuration is empty\n')
            obj.disconnect; 
            return;
        end

        %ask for ti configuration
        obj.update_ti_configuration;
        
        %wait to up to 2 seconds, or ti configuration
        start_time = tic;
        while (toc(start_time) < 2) && (isempty(fieldnames(obj.ti_configuration)))
            pause(0.05); drawnow;  %flush java events
        end

        if isempty(fieldnames(obj.ti_configuration))
            fprintf('matlab error: ti_configuration is empty\n')
            obj.disconnect; 
            return;
        end

        %ask for db configuration
        obj.update_db_configuration;

        %wait to up to 2 seconds, or db configuration
        start_time = tic;
        while (toc(start_time) < 2) && (isempty(fieldnames(obj.db_configuration)))
            pause(0.05); drawnow;  %flush java events
        end

        if isempty(fieldnames(obj.db_configuration))
            fprintf(2, 'matlab error: db_configuration is empty\n')
            obj.disconnect; 
            return;
        end

        fprintf('configuration updated and connection verified\n');
        obj.connection_verified = true;
        obj.check_disconnect_timer('reset');   %start the disconnect timer

        %print some configuration details
        fprintf('EM software version: %s   daughterboard version: %s\n', obj.ti_configuration.software_version, obj.db_configuration.daughterboard_version);
        
        %make sure to set the queue callback
        obj.add_callback('event_code_queue', @obj.packet_processor, [], true);
    end
end

function [success] = search_for_monitor(obj, serial_port)
    %first, erase all other usb connections
    obj.delete_all_serial_connections;
    
    %delete all relevant timers
    obj.delete_all_experimental_monitor_timers;

    %define the list commands
    obj.commands = struct();
    obj.commands = setfield(obj.commands, obj.list_command_string, obj.list_command_string); %#ok<SFLD>
    
    if nargin == 2 && ~isempty(serial_port) && length(serial_port) > 1
        fprintf('testing given port\n');
        success = obj.test_possible_serial_connection(serial_port); 
    else
        fprintf('searching for port that monitor is connected to\n');
        success = obj.find_serial_connection;
    end
end
