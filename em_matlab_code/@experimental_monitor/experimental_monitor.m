classdef (Sealed) experimental_monitor < handle
    %a class object to act as a front end for the Experimental Monitor
    
    
    %% public properties
    %read/write
    properties
        %basic properties
        event_codes = struct();             %blank structure, to be set up
        user_object = [];                   %could be anything
        
        %diagnostic or misc properties
        auto_disconnect_minutes = 3*60;     %3 hours
        transmit_logging = false;			%log all transmitted commands
        warn_long_callback_sec = 0.05;      %callbacks that take longer
    end
    
    %read-only properties
    properties (SetAccess = private)
        
        %poll rate
        poll_rate = 100;  %Hz
        
        %status
        is_connected = false;
        
        %commands and callbacks
        commands = struct();  %blank structure, to be set up
        callbacks = struct([]);
        packet_queue_is_empty_callback = [];
        
        %configuration (blank structures, to be retreived from em)
        ti_configuration = struct();
        db_configuration = struct();
        serial_configuration = struct();
        
        %buffers
        receive_buffer = ring_buffer(300, struct());
        error_buffer = ring_buffer(300, struct());
    end
    
    %% regular methods
    methods        
 
        %% connect and disconnect
        connect(obj, serial_port);
        disconnect(obj, force_disconnect);
        
        
        %% receive
        [callback_id] = add_callback(obj, callback_string, function_h, user_struct, silent);
        remove_callback(obj, callback_id_or_all_or_string, silent);
        
        function [] = clear_receive_buffer(obj)
            obj.receive_buffer.clear;
        end
        
        function [used_count] = receive_buffer_used(obj)
            used_count = obj.receive_buffer.used;
        end
        
        function [read_value] = read_receive_buffer(obj)
            read_value = obj.receive_buffer.read;
        end
        
        
        %% send commands
        send_command(obj, command, varargin);
        serial_transmit(obj, output, alternative_connection);
        

        %% dsp event codes and strings
        %sends codes (128-255)
        function send_event_codes_to_dsp(obj, event_codes)

            %if a string was provided, see if it exists
            if isstring(event_codes) || ischar(event_codes)
                try
                    event_codes = obj.event_codes.(event_codes);
                catch
                    fprintf('unrecognized event code name: %s\n', event_codes);
                    return;
                end

            end
            
            event_codes = uint8(event_codes);   %make sure it is uint8
            assert(isvector(event_codes))
            assert(sum(event_codes<128) == 0, 'no codes allowed below 128')
            
            json_struct = struct();
            json_struct.command = "send_event_codes";
            json_struct.codes = event_codes;
            
            try
                json_string = jsonencode(json_struct);
                obj.serial_transmit(json_string);
            catch
                fprintf('error encoding the event codes\n');
            end
        end
        
        function send_event_codes_documentation_to_dsp(obj)
            
            if ~obj.is_connected
                fprintf('not connected\n');
                return;
            end
            
            if isempty(fields(obj.event_codes))
                fprintf('error, no event_codes defined\n');
                return;
            end
            
            dsp_code_fields = fields(obj.event_codes);
            had_an_error = false;
            used_event_codes = zeros(length(dsp_code_fields), 1);
            
            for i = 1:length(dsp_code_fields)
                dsp_code_value = obj.event_codes.(dsp_code_fields{i});
                
                if ~isscalar(dsp_code_value)
                    fprintf(cat(2,'value for dsp_code ''', dsp_code_fields{i}, ''' is not a scalar\n'));
                    had_an_error = true;
                    continue;
                end
                
                code_used_before_index = find(used_event_codes == dsp_code_value);
                used_event_codes(i) = dsp_code_value;
                
                if ~isempty(code_used_before_index)
                    code_used_before_index = code_used_before_index(1);
                    fprintf(cat(2,'value for dsp_code ''', dsp_code_fields{i}, ''' is a duplicate of dsp_code ''', dsp_code_fields{code_used_before_index}, '''\n'));
                    had_an_error = true;
                    continue;
                end
                
                if dsp_code_value < 128 || dsp_code_value > 255
                    fprintf(cat(2,'value for dsp_code ''', dsp_code_fields{i}, ''' is outside range: 127<value<256\n'));
                    had_an_error = true;
                    continue;
                end
            end
            
            if had_an_error
                return;
            end
            
            obj.send_struct_to_dsp(obj.event_codes, 'event_codes');
        end
        
        
        %% send struct
        function send_struct_to_dsp(obj, struct_to_transmit, name_of_struct)
            
            if ~obj.is_connected
                fprintf('not connected\n');
                return;
            end
            
            %convert struct to packets
            fprintf('encoding structure\n');
            packets = encode_struct_to_packets(struct_to_transmit, name_of_struct);
            obj.add_packets_to_queue(packets);
        end

        function add_callback_for_empty_packet_queue(obj, function_h)
            assert(isa(function_h, 'function_handle'), 'a valid function handle must be provided')
            obj.packet_queue_is_empty_callback = function_h;
        end
        
        function [result] = is_event_code_packet_queue_empty(obj)
           result = (obj.packet_queue.used == 0);
        end
        
        
        %% helper/misc
        function [volts] = analog_value_to_volts(obj, value, varargin) %#ok<INUSL>
            volts = (value - 32768)/3200;
        end
        
        function twiddle_leds(obj)
            obj.send_command(obj.commands.twiddle_leds);
        end
        
        
        %% performance
        function change_poll_rate(obj, new_rate)
            %give warning
            if (new_rate < 20 || new_rate > 250)
                fprintf('rate expected between 20 and 250 Hz\n');
            end
            obj.poll_rate = new_rate;
            
            if (isvalid(obj.poll_serial_port_timer) & obj.poll_serial_port_timer.Running) %#ok<AND2>
                obj.start_serial_timers(false);
                obj.start_serial_timers(true);
            end
        end
        
        function synchronous_processing(obj)
            %this is for executing the rx processing without returning until done
            %NOTE: it is recommended to call delay_call('flush')
            %before entering a cpu-intensive loop that calls this function
            
            if ~obj.is_connected
                return;
            end
            
            try
                obj.poll_serial_port;   %manually poll and process
            catch e
                %should not be possible, but try to catch any errors regardless
                fprintf(2, 'error with syncronous_processing\n');
                fprintf(2, 'syncronous_processing: the identifier was: %s\n', e.identifier);
                fprintf(2, 'syncronous_processing: the message was: %s\n', e.message);
            end
        end
        
    end
    
    
    %% hidden properties and methods (not necessary for most users)
    
    %method for singleton creation
    methods (Static)
        function singleton = get_monitor
            
            if verLessThan('matlab', '9.2')
                fprintf(2, 'matlab error: this will not work on versions before 2017a\n');
                return;
            end
            
            %if no singleton, then create
            if ~isappdata(0, experimental_monitor.app_data_name)
                obj = experimental_monitor;
                setappdata(0, experimental_monitor.app_data_name, obj);
            end

            singleton = getappdata(0, experimental_monitor.app_data_name);
            
            %if there was an error, then recreate it
            if ~isa(singleton, 'experimental_monitor') || ~isvalid(singleton)
                fprintf('invalid singleton, recreating\n');
                rmappdata(0, experimental_monitor.app_data_name);   %remove bad singleton
                
                %recreate singleton
                obj = experimental_monitor;
                setappdata(0, experimental_monitor.app_data_name, obj);
                singleton = getappdata(0, experimental_monitor.app_data_name);
                
                %do a clean up
                obj.close_all_em_instruments;
                obj.delete_all_experimental_monitor_timers;
            end
        end
    end
    
    %delete method
    methods
        function delete(obj, silent)
            if nargin < 2
                silent = false;
            end
            
            %disconnect
            obj.disconnect;
            
            %remove from workspace
            base_variables = evalin('base', 'whos');
            for i = 1:length(base_variables)
                %test for eye_module
                if strcmp(base_variables(i).class, 'experimental_monitor')
                    try
                        evalin('base', cat(2, 'clear ', base_variables(i).name));
                    catch
                    end
                end
            end
            
            try
                rmappdata(0, experimental_monitor.app_data_name);
            catch
            end
            
            if ~silent
                fprintf('deleted\n');
            end
        end
    end
    
    
    %% private properties
	properties (SetAccess = private, Hidden = true)
        
        connection_verified = false;
        
        %sanity check timer
        one_per_sec_timer
        
        %rx processing
        poll_serial_port_timer
        rx_char_buffer = '';
        raw_rx_buffer = ring_buffer(300, char.empty);   %raw received char arrays
        received_struct_buffer = ring_buffer(300, struct());    %location before fully processed
        
        %serial settings and limitations
        baud_rate = 921600; %works in Matalb for macOS, Windows 10, and Ubuntu
        max_serial_string_length = 100; %default value, will be updated
        max_json_tokens = 100;  %default value, will be updated
        
        %serial objects (matlab and java)
        serial_connection;
        java_serial_object
        
        %receive
        rx_crlf_list = [];
        
        %auto disconnect
        auto_disconnect_timer
        last_transmit_timestamp = 0
        last_receive_timestamp = 0
        
        %command info
        crlf_char = cat(2,char(13),char(10)); %#ok<CHARTEN>
        command_string = 'command';
        list_command_string = 'get_commands';
        list_commands_reply = 'commands';
        
        %packet processing (as event codes)
        packet_queue = ring_buffer(10000, char.empty);  %max packets is 9999
        is_waiting_for_packet_confirmation = false;
        disconnect_on_empty_queue = false;
    end
    
    %used for the singletone storage
    properties (Constant)
        app_data_name = 'em_singleton';
    end
    
    methods (Access = private)
        function obj = experimental_monitor
        end
    end
    
    %% private methods
    methods (Access = private, Hidden = true)
        %update configuration from em
        update_command_list(obj, commands_struct, varargin);
        update_ti_configuration(obj, config_struct, varargin);
        update_db_configuration(obj, config_struct, varargin);
        update_serial_configuration(obj, config_struct, varargin);
        
        %cleanup
        delete_all_experimental_monitor_timers(obj);
        close_all_em_instruments(obj);
        delete_all_serial_connections(obj);
        
        %periodic checks
        check_disconnect_timer(obj, varargin);
        [status] = once_per_second_checks(obj, varargin);
        
        %find and test em
        [success] = find_serial_connection(obj);
        [success] = test_possible_serial_connection(obj, serial_port);
        [actually_connected] = verify_connection(obj, disconnect_if_not_connected);
        
        %serial and callback processing
        poll_serial_port(obj, varargin);
        process_rx_buffer(obj);
        start_serial_timers(obj, start_yes);
        
        %packet processing
        add_packets_to_queue(obj, packets);
        packet_processor(obj, input_struct, varargin);
    end
    
end
