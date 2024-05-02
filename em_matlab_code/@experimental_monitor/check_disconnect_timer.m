function [] = check_disconnect_timer(obj, varargin)
%checking function

%if at least a second parameter was provided
if nargin > 1
    check_varargin = varargin{1};
    if ischar(check_varargin) && strcmp(check_varargin, 'reset')

        %get the current time, quickly
        current_timestamp = datenummx(clock);

        %set last transmit and recieve to the current time
        obj.last_transmit_timestamp = current_timestamp;
        obj.last_receive_timestamp = current_timestamp;
        
        %reset the timer
        reset_autodisconnect_timer(obj);
        return;
    elseif ischar(check_varargin) && strcmp(check_varargin, 'stop')
        stop_and_delete_timer(obj);
        return;
    end
end

%get the current time, quickly
current_timestamp = datenummx(clock);

%if it already disconnected, completely disconnect
if ~obj.verify_connection(true)
    return;
end

%if either last transmit or recieve time is less than the auto
%disconnect minutes, then updat time
if ((abs(current_timestamp - obj.last_transmit_timestamp)*24*60 < obj.auto_disconnect_minutes)...
   || (abs(current_timestamp - obj.last_receive_timestamp)*24*60 < obj.auto_disconnect_minutes))
   
    reset_autodisconnect_timer(obj);
    
else
    if obj.is_connected
        %otherwise, disconnect
        fprintf('autodisconnecting\n');
        obj.disconnect;
    end
end

end

function reset_autodisconnect_timer(obj)
    
    stop_and_delete_timer(obj);
    
    %recreate the timer
    obj.auto_disconnect_timer = timer('TimerFcn', @obj.check_disconnect_timer, 'Name', 'auto_disconnect_timer', 'UserData', 'experimental_monitor');
    
    %update the start delay and stop and start the timer
    obj.auto_disconnect_timer.StartDelay = round(obj.auto_disconnect_minutes*60);
    start(obj.auto_disconnect_timer);
    
end

function stop_and_delete_timer(obj)

    try
        stop(obj.auto_disconnect_timer);
    catch
    end

    try
        delete(obj.auto_disconnect_timer);
    catch
    end

end
