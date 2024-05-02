classdef hs_recording < handle
    %object to hold the social task
    
    properties 
        channel = -1;
        channel_name = '';
        em = experimental_monitor.get_monitor;
        raw_data = [];
        history_counts = [];
        timestamps = [];
        peaks = [];
    end
    
    methods
        function obj = hs_recording()
        end
        
        function set_channel(obj, channel)
            if nargin < 1
                disp("error: must provide channel");
                return;
            end
            if channel < 0
                disp("error: channel must be 0 or greater")
                return;
            end
            obj.channel = channel;
            obj.channel_name = ['input_', num2str(channel, '%02d')];
            
            obj.em.send_command("set_input_settings", "input_number", obj.channel, "history_enabled", false);
            obj.em.send_command("set_input_settings", "input_number", obj.channel, "history_enabled", true, "history_length", 256);
            
            obj.em.remove_callback(obj.channel_name);
            obj.em.add_callback(obj.channel_name, @obj.history_callback, []);
        end

        
        function history_callback(obj, history_struct, varagin) %#ok<INUSD>
            
            if isfield(history_struct, obj.channel_name)
                channel_struct = getfield(history_struct, obj.channel_name); %#ok<GFLD>
                if isfield(channel_struct, 'history')  
                    %update to current structure
                    array_data = channel_struct.history;
                    obj.raw_data = cat(1, obj.raw_data, array_data);
                    obj.history_counts = cat(1, obj.history_counts, length(array_data));
                end
                
                if isfield(channel_struct, 'timestamp')
                    obj.timestamps = cat(1, obj.timestamps, channel_struct.timestamp);
                end
            end
        end
        
        function record (obj, seconds)
            if nargin < 2
                seconds = 1;
            end
            
            %reset all
            obj.raw_data = [];
            obj.history_counts = [];
            obj.timestamps = [];
            
            fprintf('start\n');
            obj.em.send_command("get_input_history", "input_numbers", obj.channel, "tics", 50);
            delay_call(seconds, @obj.end_recording);

            %only used for profiling
%             while ~isempty(obj.em.tx_hs_command)
%             end
        end
        
        function end_recording(obj)
            obj.em.send_command("get_input_history","stop",true);
            fprintf('done\n');
            obj.plot_data;
        end
        
        function plot_data(obj)
            
            %close other plots
            close all
            
            %make sure to get the correct frequency
            main_freq = obj.em.ti_configuration.main_freq;
            
            %raw plot
            raw_sec = (0:(length(obj.raw_data)-1))./main_freq;
            
            %peak times plot
            obj.peaks = get_peaks(double(obj.raw_data), mean(obj.raw_data));
            peak_sec = obj.peaks(2:end,1)./main_freq;
            peak_delta_sec = diff(obj.peaks(:,1))./main_freq;
            mov_avg_delta_sec = movmean(peak_delta_sec, 9);
            
            %count plot
            local_timestamps = (obj.timestamps - obj.timestamps(1));
            
            %max time
            max_time = ceil(max([max(raw_sec),max(mov_avg_delta_sec),max(local_timestamps)]));
            
            volts = obj.em.analog_value_to_volts(obj.raw_data);
            
            subplot(3,1,1)
            plot(raw_sec, volts);
            xlabel('sec')
            xlim([0, max_time]);
            ylabel('volts')
            
            subplot(3,1,2)
            plot(peak_sec, peak_delta_sec, peak_sec, mov_avg_delta_sec);
            xlabel('sec')
            xlim([0, max_time]);
            ylabel('sec')
            ylim([0, max(peak_delta_sec)*1.1])
            
            subplot(3,1,3)
            plot(local_timestamps, obj.history_counts)
            
            xlabel('sec')
            xlim([0, max_time]);
            ylabel('samples')
            ylim([0, max(obj.history_counts)]);
        end
    end
end

