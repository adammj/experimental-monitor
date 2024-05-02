classdef social_task < handle %task
    %object to hold the social task
    
    properties 
        
        em = experimental_monitor.get_monitor;
    end
    
    properties %(Hidden = true)
        has_saved = false;
        task_has_been_stopped = false;
        
        %input
        button1_pin = 0;
        button2_pin = 1;
        sense_fwd_pin = 2;
        sense_bck_pin = 3;
        
        %output
        rew1_pin = 0;
        rew2_pin = 1;
        override_pin = 2;
        direction_pin = 3;
        
        %sounds
        taskStarts
        taskEnds
        trialStarts
        trialEnds
        eating_almost_over
        taskStartsF
        taskEndsF
        trialStartsF
        trialEndsF
        eating_almost_overF
        
        %figure
        figure_h
        trial_start
        figure_update_timer
        
        %reward schedule
        nRew1
        nRew2
        condsArray
        trial_count
        
        %trial data
        social_data
        trial_i = 0;
        is_in_trial = false;
        
        %tray sense
        tray_is_bkw = 0
        tray_is_fwd = 0
        
        %button value
        button1_is_pressed = 0;
        button2_is_pressed = 0;
        both_are_pressed = 0;
        
        trial_step = 0;
        ended_session_early = false;
        
        %calibration
        last_fwd_time = 0;
        last_bkw_time = 0;
        last_pressed_together_time = 0;
        delta_time_pressed_together = 0;
        calibration_step = 0;
        calibrated_delta_fwd = 0;
        calibrated_delta_bkw = 0;
        trial_delta_fwd = [];
        predicted_delta_fwd = 0;
        override_percent = 85;
        
        %step 2 waiting
        step_2_wait_start_time = 0;
    end
    
    
    methods
        function obj = social_task
        end
        
        function connect(obj)
            obj.em.connect;
        end
        
        function calibrate_distance(obj)
            obj.em.connect;
            if obj.em.is_connected
                disp('calibrating forward and backward distance');
                obj.last_bkw_time = 0;
                obj.last_fwd_time = 0;
                obj.calibrated_delta_fwd = 0;
                obj.calibrated_delta_bkw = 0;
                
                obj.calibration_step = 0;
                
                %first move the tray a little forward
                obj.direction_fwd(true);
                obj.override_motors(true);
                
                delay_call(2, @obj.calibration_step_2);
            end
        end
        
        function calibration_step_2(obj)
            if obj.em.is_connected
                obj.calibration_step = 1;
                obj.direction_fwd(false);
            end
        end
        
        function start_task(obj)
            obj.start_automatic_figure_updating(false);
            close all
            
            obj.em = experimental_monitor.get_monitor;
            obj.em.remove_callback('all');
            obj.em.connect;
            
            if obj.em.is_connected
                try
                    if ~strcmp(obj.em.ti_configuration.launchpad_label, 'D2')
                        fprintf('the social em is D2, go find it\n');
                        obj.em.disconnect;
                        return;
                    end
                catch
                    if ~strcmp(obj.em.ti_configuration.launchpad_name, 'D2')
                        fprintf('the social em is D2, go find it\n');
                        obj.em.disconnect;
                        return;
                    end
                end
                    
                
                obj.setup_em;
                obj.load_sounds;
                
                %erase data
                obj.social_data = struct([]);
                
                %reset variables
                obj.has_saved = false;
                obj.trial_step = 0;
                obj.is_in_trial = false;
                obj.task_has_been_stopped = false;
                obj.ended_session_early = false;
            
                %create the schedule and figure
                obj.create_reward_schedule;
                obj.create_figure;
                
                
                %calibrate first
                obj.calibrate_distance;
                %after calibration run task will be called
                %obj.pre_step_0;
            else
                fprintf('not connected\n');
            end
        end
        
        function pre_step_0(obj)
            sound(obj.taskStarts, obj.taskStartsF); 
            obj.trial_delta_fwd = [];
            obj.social_data = struct([]);
            obj.trial_i = 0;
            delay_call(0, @obj.trial_step_0);
        end
               
        function trial_step_0(obj)
            if (obj.trial_i >= obj.trial_count)
                fprintf('already done\n');
                return;
            end
            
            if (obj.task_has_been_stopped)
                return;
            end
            
            %set up the trial step
            obj.trial_step = 0;
            obj.is_in_trial = false;
            obj.trial_i = obj.trial_i + 1;
            fprintf('step 0: init trial (%d/%d)\n', obj.trial_i, obj.trial_count);
            
            %store blank data for current trial
            obj.social_data(obj.trial_i, 1).b1 = [];
            obj.social_data(obj.trial_i, 1).b2 = [];
            obj.social_data(obj.trial_i, 1).rew1 = obj.nRew1(obj.trial_i);
            obj.social_data(obj.trial_i, 1).rew2 = obj.nRew2(obj.trial_i);
            obj.social_data(obj.trial_i, 1).cond = obj.condsArray(obj.trial_i);
            obj.social_data(obj.trial_i, 1).monkey_one_dispensed_first = [];
            obj.social_data(obj.trial_i, 1).trial_length = [];
            obj.social_data(obj.trial_i, 1).monkey1 = 'rum';    %this will need to be corrected if changed
            obj.social_data(obj.trial_i, 1).monkey2 = 'mo';
            obj.social_data(obj.trial_i, 1).m_pressed = false;
            %could add new fields below here
            
            %first, update the status of buttons and sense
            obj.em.serial_transmit('{"command":"get_input_settings","input_number":0,"get_value":true}');
            obj.em.serial_transmit('{"command":"get_input_settings","input_number":1,"get_value":true}');
            obj.em.serial_transmit('{"command":"get_input_settings","input_number":2,"get_value":true}');
            obj.em.serial_transmit('{"command":"get_input_settings","input_number":3,"get_value":true}');
            
            delay_call(0, @obj.trial_step_1);
        end
        
        function trial_step_1(obj)
            if (obj.task_has_been_stopped)
                return;
            end
            
            %move the tray back
            obj.trial_step = 1;
            fprintf('step 1: move tray back\n');
            
            %if tray is already back, start next step
            if obj.tray_is_bkw
                delay_call(0, @obj.trial_step_2);
            else
                %otherwise, move it back
                title('moving tray back');
                obj.direction_fwd(false);
                obj.override_motors(true);
            end
        end
        
        function trial_step_2(obj)
            if (obj.task_has_been_stopped)
                return;
            end
            
            %prepare to dispense pellets step
            obj.trial_step = 2;
            fprintf('step 2: wait to dispense pellets\n');
            
            %stop overriding
            obj.override_motors(false);
            
            %desired_wait = 5;
            obj.step_2_wait_start_time = tic;
           
            delay_call(0, @obj.check_step_2);
        end
        
        function check_step_2(obj)
            desired_wait = 5;
            
            %if either are pressed, just keep resetting
            if obj.button1_is_pressed || obj.button2_is_pressed
                obj.step_2_wait_start_time = tic;
            end
            
            if toc(obj.step_2_wait_start_time) < desired_wait
                title(['trial ', num2str(obj.trial_i), ': ' num2str(desired_wait - toc(obj.step_2_wait_start_time), '%0.1f') ' seconds left before starting']);
                drawnow;
                delay_call(0.2, @obj.check_step_2);
                return;
            end
            
            %move to next step
            delay_call(0, @obj.trial_step_3);
        end
        
        function trial_step_3(obj)
            if (obj.task_has_been_stopped)
                return;
            end
            
            %begin the main part of the trial, and dispense pellets
            obj.trial_step = 3;
            fprintf('step 3: start main part of trial\n');
            
            %restart the experiment count
            obj.last_pressed_together_time = 0;
            obj.delta_time_pressed_together = 0;
            obj.is_in_trial = true;
            obj.em.send_command("reset_experiment_clock");
            obj.trial_start = now();
            
            %dispense rewards
            obj.dispense_rewards_for_current_trial;
            
            %now that the pellets have been dispensed, change the direction to
            %forward (allow the tray to move when the monkeys press)
            obj.direction_fwd(true);
            sound(obj.trialStarts, obj.trialStartsF);
            title(['trial ' num2str(obj.trial_i) ' (' num2str(obj.nRew1(obj.trial_i)) ', ' num2str(obj.nRew2(obj.trial_i)) '): trial started']);
            drawnow;
            
            obj.start_automatic_figure_updating(true);
        end
        
        function start_automatic_figure_updating(obj, start_yes)
            if start_yes
                %first stop the figure
                obj.start_automatic_figure_updating(false);
                
                %then create and start the timer
                obj.figure_update_timer = timer('TimerFcn', @obj.update_figure_clock, 'Name', 'update_figure_timer', 'BusyMode', 'drop', 'ExecutionMode', 'fixedRate', 'Period', 0.5);
                %fprintf('start update timer\n');
                start(obj.figure_update_timer)
            else
                try
                    %fprintf('stop update timer\n');
                    stop(obj.figure_update_timer);
                    %fprintf('delete update timer\n');
                    delete(obj.figure_update_timer);
                catch
                end
           end
           
        end
        
        function update_figure_clock(obj, varargin)
            
            if ~obj.em.is_connected
               fprintf(2, 'em is no longer connected, stopping task\n');
               delay_call(0, @obj.stop_task);
            end
            
            trial_time = (now() - obj.trial_start)*3600*24;
            title(['trial ' num2str(obj.trial_i) ' (' num2str(obj.nRew1(obj.trial_i)) ', ' num2str(obj.nRew2(obj.trial_i)) '): trial started (' num2str(trial_time, '%0.1f') ' sec)']);
            drawnow;
        end
        
        function trial_step_4(obj)
            if (obj.task_has_been_stopped)
                return;
            end
            
            %trays have reached the front, finish up
            obj.trial_step = 4;
            fprintf('step 4: finish up trial (%d/%d)\n', obj.trial_i, obj.trial_count);
            
            %first, override and move tray forward
            obj.override_motors(true);
            
            %stop the update timer
            obj.start_automatic_figure_updating(false);
            drawnow;
            
            %play sound and let monkeys eat
            sound(obj.trialEnds, obj.trialEndsF);            
            obj.figure_h.UserData = 0;
            title(['trial ' num2str(obj.trial_i) ' (' num2str(obj.nRew1(obj.trial_i)) ', ' num2str(obj.nRew2(obj.trial_i)) '): waiting to eat'])
            
            
            %if not the last, start over
            if (obj.trial_i >= obj.trial_count) || obj.ended_session_early
                fprintf('session over\n');
                delay_call(0, @obj.stop_task);
            else
                fprintf('wait for them to eat\n');
                delay_call(10, @obj.notify_almost_over);
            end
        end
        
        function notify_almost_over(obj, varargin)
            fprintf('notify tray is about to move\n');
            sound(obj.eating_almost_over, obj.eating_almost_overF);
            
            fprintf('wait to start next trial\n');
            delay_call(2, @obj.trial_step_0);
        end
        
        function stop_task(obj)
            %need to kill all timers here, and also have a catch in each
            %function for the task is over (a variable)
            
            obj.task_has_been_stopped = true;
            obj.em.remove_callback('all');
            
            %stop the figure update
            obj.start_automatic_figure_updating(false);
                
            if ~obj.has_saved
                fprintf('preparing to play finish music and save\n');
                delay_call(2, @obj.play_finish_and_save);
            else
                fprintf('already saved\n'); 
            end
        end
        
        function play_finish_and_save(obj, varargin)
            obj.override_motors(false);
            fprintf('finished\n');
            title('finished!')
            sound(obj.taskEnds, obj.taskEndsF);            
            obj.save_data;
        end
        
        function dispense_rewards_for_current_trial(obj)
            if (obj.trial_i == 0)
                fprintf('trial is 0, error\n');
                return;
            end
            
            title(['trial ' num2str(obj.trial_i) ' (' num2str(obj.nRew1(obj.trial_i)) ', ' num2str(obj.nRew2(obj.trial_i)) '): dispensing']);
            obj.give_reward_to_side1(obj.nRew1(obj.trial_i));
            obj.give_reward_to_side2(obj.nRew2(obj.trial_i));
        end
        
        function create_reward_schedule(obj)
            ValueMatrix = [2 2;2 3;3 2;3 3];
            
            nconds = [15;10;10;15]; %normal
            %nconds = [6;6;6;6]; %half
            
            %nconds = [1;1;1;1];    %short
            
            obj.trial_count = sum(nconds);

            obj.condsArray = zeros(1,obj.trial_count);
            index1 = randperm(obj.trial_count, nconds(1));
            obj.condsArray(index1) = 1;

            index0 = setdiff(1:obj.trial_count,index1);
            index2 = randperm(length(index0),nconds(2));
            obj.condsArray(index0(index2)) = 2; 

            index03 = find(obj.condsArray == 0);
            index3 = randperm(length(index03), nconds(3)); 
            obj.condsArray(index03(index3)) = 3;

            index04 = find(obj.condsArray == 0); 
            index4 = randperm(length(index04), nconds(4)); 
            obj.condsArray(index04(index4)) = 4;
            % 
            % index5 = find(condsArray == 0);
            % condsArray(index5) = 5;

            obj.nRew1 = ValueMatrix(obj.condsArray, 1);
            obj.nRew2 = ValueMatrix(obj.condsArray, 2);
        end
        
        
        function create_figure(obj)
            close all;
            obj.figure_h = figure;
            set(obj.figure_h, 'WindowKeyPressFcn', @obj.process_key_events);
            hold on
            obj.figure_h.UserData = 0;
            ylim([0 obj.trial_count]);
            xlim([0 300])
        end
        
        function save_data(obj)
            disp('saving...')            
            %create local varaible to save correctly
            social_data = obj.social_data; %#ok<NASGU,PROP>
            save(['rewEq_' datestr(now,'yyyy-mm-dd_HH_MM_SS') '.mat'], 'social_data');
            disp('saved')

            fprintf('total rewards m1: %i   m2: %i\n', sum(obj.nRew1), sum(obj.nRew2));
        end
        
        function setup_em(obj)
            
            %fprintf('add callbacks\n');
            obj.em.add_callback('input_00', @obj.input_callback);
            obj.em.add_callback('input_01', @obj.input_callback);
            obj.em.add_callback('input_02', @obj.input_callback);
            obj.em.add_callback('input_03', @obj.input_callback);
            
            %set rewards
            obj.em.send_command("set_output_settings", "output_number", 0, "on_tics", 250, "off_tics", 250, "on_value", 1, "off_value", 0, "set_value", 0);
            obj.em.send_command("set_output_settings", "output_number", 1, "on_tics", 250, "off_tics", 250, "on_value", 1, "off_value", 0, "set_value", 0);
            
            %set button inputs
            obj.em.send_command("set_input_settings", "input_number", 0, "send_to_computer", true, "target_met_min_tics", 1, "target_left_min_tics", 1, "actions_enabled", true, "target", 1,"all_transitions", true);
            obj.em.send_command("set_input_settings", "input_number", 1, "send_to_computer", true, "target_met_min_tics", 1, "target_left_min_tics", 1, "actions_enabled", true, "target", 1,"all_transitions", true);
            
            %set sense 
            %fwd
            obj.em.send_command("set_input_settings", "input_number", 2, "send_to_computer", true, "target_met_min_tics", 1, "target_left_min_tics", 1, "actions_enabled", true, "target", 1, "all_transitions", true);
            %bkw
            obj.em.send_command("set_input_settings", "input_number", 3, "send_to_computer", true, "target_met_min_tics", 1, "target_left_min_tics", 1, "actions_enabled", true, "target", 1, "all_transitions", true);
        end
        
        function load_sounds(obj)
            [obj.taskStarts, obj.taskStartsF] = audioread('nsmb_course_clear-bonus.wav');
            [obj.taskEnds, obj.taskEndsF] = audioread('smb3_level_clear.wav');        
            [obj.trialStarts, obj.trialStartsF] = audioread('smb_powerup.wav');
            [obj.trialEnds, obj.trialEndsF] = audioread('smb_coin.wav');
            [obj.eating_almost_over, obj.eating_almost_overF] = audioread('smb_bump.wav');
        end
        
        function give_reward_to_side1(obj, count)
            if nargin < 2
                fprintf('must provide count\n');
                return;
            end
            %fprintf('give give_reward_to_side1: %i\n', count); 
            string = cat(2,'{"command":"set_output_actions","output_number":0,"add_cycles":', num2str(count),'}');
            obj.em.serial_transmit(string);
        end
        
        function give_reward_to_side2(obj, count)
            if nargin < 2
                fprintf('must provide count\n');
                return;
            end
            %fprintf('give give_reward_to_side2: %i\n', count);
            string = cat(2,'{"command":"set_output_actions","output_number":1,"add_cycles":', num2str(count),'}');
            obj.em.serial_transmit(string);
        end
        
        function direction_fwd(obj, fwd_yes)
            %fprintf('direction_fwd: %i\n', fwd_yes);
            if fwd_yes
                obj.em.serial_transmit('{"command":"set_output_settings","output_number":3,"set_value":1}');
            else
                obj.em.serial_transmit('{"command":"set_output_settings","output_number":3,"set_value":0}');
            end
        end
        
        function override_motors(obj, yes_override)
            %fprintf('override_motors: %i\n', yes_override);
            if yes_override
                obj.em.serial_transmit('{"command":"set_output_settings","output_number":2,"set_value":1}');
            else
                obj.em.serial_transmit('{"command":"set_output_settings","output_number":2,"set_value":0}');
            end
        end
        
        function input_callback(obj, callback_struct, varargin)
            %handles all callbacks
            
            if isfield(callback_struct, 'input_00')
                %button 1 pressed
                
                seconds = callback_struct.input_00.timestamp;
                
                %if in step 2, reset the start time
                if obj.trial_step == 2
                    obj.step_2_wait_start_time = tic;
                end
                
                
                %this is returned when get_value is called
                if isfield(callback_struct.input_00, 'value')
                    obj.button1_is_pressed = callback_struct.input_00.value;
                end
                
                if isfield(callback_struct.input_00, 'target_met')
                    met = callback_struct.input_00.target_met;
                    %fprintf('button1: met: %d time: %f\n', met, seconds);
                    
                    obj.button1_is_pressed = met;
                    
                    if obj.trial_step == 3
                        if (obj.button1_is_pressed && obj.button2_is_pressed)
                            obj.both_are_pressed = true;
                            obj.last_pressed_together_time = seconds;
                            fprintf('both pressed at: %f\n', seconds);
                        end

                        if (~obj.button1_is_pressed || ~obj.button2_is_pressed) && obj.both_are_pressed
                            obj.both_are_pressed = false;
                            fprintf('one let go at: %f\n', seconds);
                            if obj.last_pressed_together_time > 0
                                delta = seconds - obj.last_pressed_together_time;
                                obj.delta_time_pressed_together = obj.delta_time_pressed_together + delta;
                                estimated_percent = obj.delta_time_pressed_together/obj.predicted_delta_fwd*100;
                                fprintf('delta length: %.3f, total delta: %.3f, estimated percent: %.1f%%\n', delta, obj.delta_time_pressed_together, estimated_percent);

                                if estimated_percent >= obj.override_percent
                                    %direction is already forward, now override motor
                                    obj.last_pressed_together_time = seconds;
                                    obj.override_motors(true);
                                end
                            end
                        end
                    end
                   
                    if obj.trial_i > 0
                        trial_time = seconds;
                        temp_entry = [trial_time, met];
                        obj.social_data(obj.trial_i, 1).b1 = cat(1, obj.social_data(obj.trial_i, 1).b1, temp_entry);
                        %plot
                        if obj.is_in_trial
                            if obj.button1_is_pressed
                                plot(trial_time, obj.trial_i, 'r*');   %filled for on
                            else
                                plot(trial_time, obj.trial_i, 'ro');   %empty for off
                            end
                        end
                    end
                end
                
            elseif isfield(callback_struct, 'input_01')
                %button 2 pressed
                
                seconds = callback_struct.input_01.timestamp;
                
                %if in step 2, reset the start time
                if obj.trial_step == 2
                    obj.step_2_wait_start_time = tic;
                end

                %this is returned when get_value is called
                if isfield(callback_struct.input_01, 'value')
                    obj.button2_is_pressed = callback_struct.input_01.value;
                    %fprintf('button2 v: %d\n', obj.button2_is_pressed);
                end
                
                if isfield(callback_struct.input_01, 'target_met')
                    met = callback_struct.input_01.target_met;
                	%fprintf('button2: met: %d time: %f\n', met, seconds);
                    obj.button2_is_pressed = met;
                    
                    if obj.trial_step == 3
                        if (obj.button1_is_pressed && obj.button2_is_pressed)
                            obj.both_are_pressed = true;
                            obj.last_pressed_together_time = seconds;
                            fprintf('both pressed at: %f\n', seconds);
                        end

                        if (~obj.button1_is_pressed || ~obj.button2_is_pressed) && obj.both_are_pressed
                            obj.both_are_pressed = false;
                            fprintf('one let go at: %f\n', seconds);
                            
                            if obj.last_pressed_together_time > 0
                                delta = seconds - obj.last_pressed_together_time;
                                obj.delta_time_pressed_together = obj.delta_time_pressed_together + delta;
                                estimated_percent = obj.delta_time_pressed_together/obj.predicted_delta_fwd*100;
                                fprintf('delta length: %.3f, total delta: %.3f, estimated percent: %.1f%%\n', delta, obj.delta_time_pressed_together, estimated_percent);

                                if estimated_percent >= obj.override_percent
                                    %direction is already forward, now override motor
                                    obj.last_pressed_together_time = seconds;
                                    obj.override_motors(true);
                                end
                            end
                        end
                    end
                   
                    if obj.trial_i > 0
                    	trial_time = seconds;
                        temp_entry = [trial_time, met];
                        obj.social_data(obj.trial_i, 1).b2 = cat(1, obj.social_data(obj.trial_i, 1).b2, temp_entry);
                        %plot
                        if obj.is_in_trial
                            if obj.button2_is_pressed
                                plot(trial_time, obj.trial_i, 'g*');   %filled for on
                            else
                                plot(trial_time, obj.trial_i, 'go');   %empty for off
                            end
                        end
                    end
                end
            
            elseif isfield(callback_struct, 'input_02')    
                %hit front wall

                seconds = callback_struct.input_02.timestamp;

                %this is returned when get_value is called
                if isfield(callback_struct.input_02, 'value')
                    obj.tray_is_fwd = (callback_struct.input_02.value == 0);
                    %fprintf('sense_fwd v: %d\n', obj.tray_is_fwd);
                end
                
                %this is returned when a target is met or left
                if isfield(callback_struct.input_02, 'target_met')
                    obj.tray_is_fwd = (callback_struct.input_02.target_met == 0);
                    %fprintf('sense_fwd m: %d time: %f\n', obj.tray_is_fwd, seconds);
                end
                
                if obj.tray_is_fwd
                    fprintf('tray is against front, at time: %f\n', seconds);
                    obj.last_fwd_time = seconds;

                    if obj.calibration_step == 2
                        obj.calibration_step = 3;    
                        obj.calibrated_delta_fwd = seconds - obj.last_bkw_time;
                        obj.predicted_delta_fwd = obj.calibrated_delta_fwd;
                        fprintf('setting last_fwd_time, and changing direction (calibrated_delta_fwd = %f)\n',  obj.calibrated_delta_fwd);

                        %now that we have the fwd time, move motor foward
                        obj.direction_fwd(false);
                    end

                    %if the tray is forward, and the step is 3, move to step 4
                    if obj.trial_step == 3
                        
                        %if during the trial
                        if obj.last_pressed_together_time > 0
                            delta = seconds - obj.last_pressed_together_time;
                            obj.delta_time_pressed_together = obj.delta_time_pressed_together + delta;
                            fprintf('delta length: %f, final total delta: %f\n', delta, obj.delta_time_pressed_together);
                            if ~isempty(obj.trial_i)
                                obj.trial_delta_fwd(obj.trial_i, 1) = obj.delta_time_pressed_together;
                                %update the predicted_delta_fwd
                                obj.predicted_delta_fwd = median(cat(1, obj.calibrated_delta_fwd, obj.trial_delta_fwd));
                            end
                        end

                        %if a valid trial, and tray is forward, record time
                        if obj.trial_i > 0 %&& isempty(obj.social_data(obj.trial_i, 1).trial_length)
                            trial_length = seconds;
                            obj.social_data(obj.trial_i,1).trial_length = trial_length;
                            plot(trial_length, obj.trial_i, 'b+');   %plus sign for end of trial
                        end

                        fprintf('move to step 4\n');
                        delay_call(0, @obj.trial_step_4);
                    end
                end
                    
            elseif isfield(callback_struct, 'input_03')
                %hit back wall

                seconds = callback_struct.input_03.timestamp;
                
                %this is returned when get_value is called
                if isfield(callback_struct.input_03, 'value')
                    obj.tray_is_bkw = (callback_struct.input_03.value == 0);
                    %fprintf('sense_bkw v: %d\n', obj.tray_is_bkw);
                end
                
                %this is returned when a target is met or left
                if isfield(callback_struct.input_03, 'target_met')
                    obj.tray_is_bkw = (callback_struct.input_03.target_met == 0);
                    %fprintf('sense_bkw m: %d time: %f\n', obj.tray_is_bkw, seconds);              
                end
                
                if obj.tray_is_bkw
                    fprintf('tray is against back, at time: %f\n', seconds);
                    obj.last_bkw_time = seconds;

                    if obj.calibration_step == 1
                        obj.calibration_step = 2;
                        disp('setting last_bkw_time, and changing direction')

                        %now that we have the fwd time, move motor foward
                        obj.direction_fwd(true);
                    elseif obj.calibration_step == 3
                        obj.calibrated_delta_bkw = seconds - obj.last_fwd_time;
                        fprintf('stopping calibration (calibrated_delta_bkw = %f)\n',  obj.calibrated_delta_bkw);
                        obj.override_motors(false);
                        obj.direction_fwd(true);

                        %done calibrating
                        obj.calibration_step = 0;
                        
                        disp('done calibratin. now, run the task.');
                        delay_call(0, @obj.pre_step_0);
                    end

                    %if the tray is now bwk, and it was step 1, move to step 2
                    if obj.trial_step == 1
                        fprintf('move to step 2\n');
                        delay_call(0, @obj.trial_step_2);
                    end
                end
                
            
            else
                fprintf('unknown callback struct\n');
                fieldnames(callback_struct)
            end
            
        end
        
        function process_key_events(obj, hFig, eventdata, varargin)
            
            if(eventdata.Key == 's')
                hFig.UserData = 1;
                
            elseif(eventdata.Key == 'e')
                hFig.UserData = 2;
                
            elseif(eventdata.Key == 'b')
                hFig.UserData = 3;
                
            elseif(eventdata.Key == 'm')
                %hFig.UserData = 'm';    
                
                if obj.trial_step == 3
                    fprintf('end trial override\n');
                    %set the note for future
                    obj.social_data(obj.trial_i, 1).m_pressed = true;

                    %clear user data
                    %obj.figure_h.UserData = 0;

                    %direction is already forward, now override motor
                    obj.override_motors(true);
                end
                
            elseif(eventdata.Key == 'x')
                %hFig.UserData = 'x';
                
                if obj.trial_step == 3
                    fprintf('end session override\n');
                    %set the note for future
                    obj.social_data(obj.trial_i, 1).m_pressed = true;

                    %clear user data
                    %obj.figure_h.UserData = 0;

                    %direction is already forward, now override motor
                    obj.override_motors(true);
                    
                    obj.ended_session_early = true;
                    %obj.trial_i = obj.trial_count;  %and set the trial as over
                end
            end
        end
    end
end
