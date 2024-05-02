
pool = gcp('nocreate');
if isempty(pool)
    pool = parpool(2); % creating parallel pool with 2 workers
end

if ~exist('queue_to_client')
    queue_to_client = parallel.pool.DataQueue; % initializing a DataQueue in Parallel pool for workers to send back acquired data to client

    %afterEach(q, @(data) plot(data));  % after each data set is received by client, it would call the plot function
    %afterEach(q, @get_em);
    afterEach(queue_to_client, @status_msg);
end

disp('eval')
feval_h = parfeval(pool, @daqInputSession, 1, queue_to_client, '');  % evaluating function 'daqInputSession' on the Parallel pool
%f.State
disp('called')
%f.State
%getappdata(0)

clear queue_to_worker
while isempty(feval_h.OutputArguments)
    pause(0.01);
end
queue_to_worker = feval_h.OutputArguments{1};

% function get_em(obj)
%     fprintf('get_em\n');
%     assignin('base','em',obj);
% end

function [q_to_worker] = daqInputSession(q_to_client, varargin)
    clc
    
    em = experimental_monitor.get_monitor;
    
    if nargin > 1 && ~isempty(varargin)
       if ischar(varargin{1})
           if strcmp(varargin{1}, 'disconnect')
               em.disconnect;
           end
       end
    end
    
    if ~em.is_connected
        fprintf('connecting\n');
        em.set_daughterboard(3);
        em.connect;
    else
        fprintf('connected\n');
    end
    
    
    delay_call('status')
    delay_call('reset')
    delay_call(5, @inner);
    %delay_call(10, @inner);
    
    em.user_object = struct();
    
    if isempty(em.user_object) || isempty(fields(em.user_object))
        q_to_worker = parallel.pool.DataQueue;
        em.user_object.q_to_worker = q_to_worker;
        em.user_object.q_to_client = q_to_client;
        afterEach(q_to_worker, @inner);
        
    else
        q_to_worker = em.user_object.q_to_worker;
    end
    
    
    try
        output_cell = command_window_text;
        em.user_object.q_to_client.send(output_cell);
    catch
        em.user_object.q_to_client.send('error getting text');
    end
    
    em.user_object.q_to_client.send('done');
    
    %q.send(q_to_worker);
    %q.send('done');
    
    function inner(varargin)
        em_local = experimental_monitor.get_monitor;
        
        if ~isempty(varargin)
            v = varargin{1};
            em_local.user_object.q_to_client.send(v);
            switch(v)
                case 'connect'
                    em_local.connect;
                    st = tic;
                    while toc(st) < 5 && ~em_local.is_connected
                        pause(0.01);
                    end
                    
                    if em_local.is_connected
                        em_local.user_object.q_to_client.send('connected');
                    else
                        em_local.user_object.q_to_client.send('fail');
                    end
                case 'disconnect'
                    em_local.disconnect;
                case 'twiddle'
                    em_local.twiddle_leds;

                otherwise
                    em_local.twiddle_leds;
            end
            em_local.user_object.q_to_client.send(delay_call('status'));
        else
        
            %em_local.user_object.q_to_client.send('twiddle');
            em_local.user_object.q_to_client.send(delay_call('status'));
            em_local.twiddle_leds;
        end
        
        try
            output_cell2 = command_window_text;
            em_local.user_object.q_to_client.send(output_cell2);
        catch
            em.user_object.q_to_client.send('error getting text');
        end
        
    end

end