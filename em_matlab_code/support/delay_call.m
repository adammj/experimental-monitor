function [has_error_output] = delay_call(seconds_or_flag, function_h, varargin)
%calls a function after some delay, and cleans up after it has run
%can use a flag to clean up or get status
%once one fatal error is encountered, it will not allow anymore until flag
%is fixed

persistent has_fatal_error

if isempty(has_fatal_error)
    has_fatal_error = false;
end

has_error_output = has_fatal_error;

%can clean up all delay_call timers
if ischar(seconds_or_flag)
    if strcmp(seconds_or_flag, 'clean')
        cleanup_all_delay_call_timers;
    elseif strcmp(seconds_or_flag, 'reset')
        has_fatal_error = false;
        has_error_output = false;   %update status returned
    elseif strcmp(seconds_or_flag, 'status')
        fprintf('delay_call has an error: %s\n', mat2str(has_fatal_error));
    elseif strcmp(seconds_or_flag, 'flush')
        flush_all_delay_calls;
    end
    
    return;
end

%don't run again until error is solved
if has_fatal_error
    return;
end

assert(isa(function_h, 'function_handle'), 'a valid function handle must be provided');
assert(isscalar(seconds_or_flag), 'seconds must be a single number');
assert(seconds_or_flag >= 0, 'seconds must be >= 0');
seconds_or_flag = round(seconds_or_flag, 3); %timer only allows ms accuracy

%store the function and varargin list
combined_struct = struct();
combined_struct.function_h = function_h;
combined_struct.varargin_list = varargin;

try
    %this creation method is about 20% faster than the method below
    new_timer_object = timer;
    new_timer_object.TimerFcn = @delay_call_function;
    new_timer_object.Name = 'delay_call'; 
    new_timer_object.StartDelay =seconds_or_flag;
    new_timer_object.UserData = combined_struct;
    new_timer_object.ErrorFcn = @delay_call_error; 
    new_timer_object.StopFcn = @delay_call_stopped;
    
    %timer_object = timer('TimerFcn', @delay_call_function, 'Name', 'delay_call', 'StartDelay', seconds_or_flag, 'UserData', combined_struct, 'ErrorFcn', @delay_call_error, 'StopFcn', @delay_call_stopped);
catch
    %this is could be a fatal error
    has_fatal_error = true;
    fprintf(2, 'delay_call: cannot create timer\n');
    return;
end


try
    start(new_timer_object)
catch
    %this is could be a fatal error
    has_fatal_error = true;
    fprintf(2, 'delay_call: cannot start timer\n');
    return;
end

%exit, nothing else left to do
return;

%% timer callback functions (nested inside to get has_fatal_error variable)

    %called at desired delay
    function delay_call_function(local_timer_object, varargin)
        %must copy the struct locally
        combined_struct_local = local_timer_object.UserData;

        %call the function
        try
            feval(combined_struct_local.function_h, combined_struct_local.varargin_list{:});
        catch e
            %this is not a fatal error, probably an error with the function
            fprintf(2, 'delay_call: error with function: %s\n', get_function_string(combined_struct_local.function_h));
            fprintf(2, 'delay_call: the identifier was: %s\n', e.identifier);
            fprintf(2, 'delay_call: the message was: %s\n', e.message);
        end
    end

    %called after timer has stopped
    function delay_call_stopped(timer_object, varargin)
        try
            delete(timer_object);
        catch e
            %this is could be a fatal error
            has_fatal_error = true;
            combined_struct_local = timer_object.UserData;
            fprintf(2, 'delay_call: unable to clean up timer for function: %s\n', get_function_string(combined_struct_local.function_h));
            fprintf(2, 'delay_call: the identifier was: %s\n', e.identifier);
            fprintf(2, 'delay_call: the message was: %s\n', e.message);
        end
    end

    %called if timer has an error
    function delay_call_error(timer_object, varargin)
        %this is could be a fatal error
        has_fatal_error = true;
        
        combined_struct_local = timer_object.UserData;
        fprintf(2, 'delay_call: there was an error for function: %s\n', get_function_string(combined_struct_local.function_h)); 
        varargin %#ok<NOPRT>
    end

    %function for cleanup
    function cleanup_all_delay_call_timers()

        %safely clean up all stopped delay_calls
        try
            all_timers = timerfindall';
            if ~isempty(all_timers)
                delete(all_timers(strcmp({all_timers.Name}','delay_call') & strcmp({all_timers.Running}','off')));
            end
        catch e
            %this is could be a fatal error
            has_fatal_error = true;
            fprintf(2, 'delay_call: unable to cleanup all stopped delay_call timers\n');
            fprintf(2, 'delay_call: the identifier was: %s\n', e.identifier);
            fprintf(2, 'delay_call: the message was: %s\n', e.message);
        end
    end

    function flush_all_delay_calls()
        %flush takes about 1ms if nothing to flush
        try
            all_timers = timerfindall';
            if ~isempty(all_timers)
                for i = 1:size(all_timers,1)
                    temp_timer_object = all_timers(i);
                    
                    %if it is a delay_call, is running, and hasn't been executed, then force it to execute now
                    if strcmp(temp_timer_object.Name, 'delay_call') && strcmp(temp_timer_object.Running, 'on') && (temp_timer_object.TasksExecuted == 0)
                        %first, must copy the struct locally
                        combined_struct_local = temp_timer_object.UserData;

                        %second, stop the timer (after copy),
                        %which also deletes it
                        stop(temp_timer_object)
                        
                        %call the function
                        try
                            feval(combined_struct_local.function_h, combined_struct_local.varargin_list{:});
                        catch e
                            %this is not a fatal error, probably an error with the function
                            fprintf(2, 'delay_call: error with function: %s\n', get_function_string(combined_struct_local.function_h));
                            fprintf(2, 'delay_call: the identifier was: %s\n', e.identifier);
                            fprintf(2, 'delay_call: the message was: %s\n', e.message);
                        end
                    end
                end
            end
            
        catch e
            %this is could be a fatal error
            has_fatal_error = true;
            fprintf(2, 'delay_call: unable to flush delay_call\n');
            fprintf(2, 'delay_call: the identifier was: %s\n', e.identifier);
            fprintf(2, 'delay_call: the message was: %s\n', e.message);
        end
    end

    function function_str = get_function_string(function_h)
        function_str = func2str(function_h);
        function_str = strrep(function_str, '@(varargin)', '');
        function_str = strrep(function_str, '(varargin{:})', '');
    end
end
