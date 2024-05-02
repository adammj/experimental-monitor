classdef (Sealed) eye_module < handle
    %eye_module (there can be only one)
    
    properties
        refresh_rate = 20;  %in Hz
        counts = [];
    end
    
    properties (SetAccess = private)
        em = experimental_monitor.get_monitor;
%         hs_command = '';
        inputs = struct();
        reward_channel = -1;
        
        %calibration (must be set through functions)
        monitor_specs = struct();
        monkey_distance_mm = 0;
        has_dimensions = false;
        circle_fixation_points = struct([]);
        fixation_diameter = struct('deg', [], 'raw', 1000);
        screen_fixation_diameter_deg = 0.5;
        fixation_r_deg = 10;
        fixation_theta_deg = 0;
        current_circle_fixation = [];
        
        %calibration_points = [];    %stores the calibration points (not sure how to use yet)
        is_calibrated = false;
        is_calibrating = false;
        calibration = struct();
        is_streaming = false;   %default to off
        history_seconds = 0;
        stop_repeat = false;
        
        
        x_callback_id = 0;
        y_callback_id = 0;
        
        reward_on_tics = 500;
        reward_off_tics = 500;
        reward_output_is_reversed = false;
    end
    
    properties (Hidden)
        %constants (to make it easier to change later)
        raw_min = 0;
        raw_middle = 32768;
        raw_max = 65535;
        
    end
    
    %% control panel properties
    properties
        control_panel_ready = false;
        running_test = false
        
        %plotting
        control_panel_h;    %main window
        stream_button_h;    %streaming button
        cp_dims_cm = [];
        cp_scale = 1;
        current_axes_h;     %axes for plot
        %avg_time = 0.001;
        
        %plot extents
        plot_radius
        plot_center
        x_range;
        y_range;
        
        %eye scatter
        eye_scatter_h
        %eye_scatter_refresh_hz = 50;
        eye_scatter_tail_sec = 0;
        
        %current point
        selected_circle_h   %circle for selecting the cloud
        selected_center
        %current_fixation_pt     %current_selected_pt
        
        %degree cicles (might not use)
        degree_circle1_h
        degree_circle2_h
        degree_circle3_h
        degree_circle4_h
        degree_circle5_h
        
        %fixation points
        array_of_fixation_circles = struct([]);    %array of handles
        
        %monitor shape (could be useful)
        monitor_shape_h
        
        %support for circle plots
        circle_thetas = 0:pi/20:2*pi;
        
        %no longer used
%         eye_initial_tic
%         eye_last_tic;
        
        %framerate = 0;
        %eye_update_timer;
        %eye_update_plot_count = 20;
        %callback_id = 0;    %default value
        %frame_count = 0;
        %is_displaying_eye_tracking = false;
        %lengths = [];
        
                %temp
%         x_values
%         y_values
    end
    
    %% method for singleton creation and access
    
    methods (Static)
        function singleton = get_module
            
            if verLessThan('matlab', '9.2')
                fprintf(2, 'matlab error: this will not work on versions before 2017a\n');
                return;
            end
            
            %this keeps a really persistent object
            if ~isappdata(0, eye_module.app_data_name)
                obj = eye_module;
                setappdata(0, eye_module.app_data_name, obj);
            end

            singleton = getappdata(0, eye_module.app_data_name);
            
            singleton.em = experimental_monitor.get_monitor; %update the handle
        end
    end
    
    
    %% calibration methods
    methods
        
        function reset_counts(obj)
            obj.counts = datenummx(clock);
        end
        
        function plot_counts(obj)
            times = obj.counts.*3600*24;
            times(isempty(times)) = [];
            times = unique(times);
            times = times - times(1);
            figure
            plot(times(2:end), diff(times));
        end
        
        function refresh_callbacks(obj)
            not_setup = false;
            try
                if isempty(obj.inputs.x.input_string) || isempty(obj.inputs.x.input_string)
                    not_setup = true;
                end
            catch
                not_setup = true;
            end
            
            if not_setup
                fprintf('io_channels have not been set yet\n');
                return;
            end
            
            %set the callbacks
            %will automatically remove old callbacks to same function signature
            obj.x_callback_id = obj.em.add_callback(obj.inputs.x.input_string, @obj.cp_plot_update, [], true);
            obj.y_callback_id = obj.em.add_callback(obj.inputs.y.input_string, @obj.cp_plot_update, [], true);
        end
        
        function test_stream_eye(obj)
            obj.em.clear_receive_time_buffer
            pause(2.5);
            obj.stream_eye(true);
            pause(5);
            obj.stream_eye(false);
            pause(2.5);
            obj.em.plot_receive_time_buffer;
        end
        
        function stream_eye(obj, yes_no, new_rate)
            if nargin > 2
                obj.refresh_rate = new_rate;
            end
            
            if ~yes_no
                %it isn't streaming, so ignore
                if ~obj.is_streaming
                    return;
                end
                
                fprintf('stop streaming\n')
                obj.em.send_command("get_input_history","stop",true);
                obj.is_streaming = false;
                %extra delayed called to clear the cloud
                delay_call(0.25, @obj.clear_eye_cloud, obj);
                
                %if the panel is open, then make sure to set the button
                if obj.is_control_panel_open
                    obj.set_button_on(obj.stream_button_h, false);
                end
                return;
            end
            
            if ~obj.is_control_panel_open
                fprintf('control panel not open\n');
                return;
            end
            
            %must check everything beforehand
            fprintf('start streaming\n')
            obj.set_button_on(obj.stream_button_h, true);
            
            input_array = [obj.inputs.x.number, obj.inputs.y.number];
            tics = round(1000/obj.refresh_rate);
            obj.em.send_command("get_input_history", "input_numbers", input_array, "tics", tics);
            
            obj.is_streaming = true;
        end
        
        function set_dimensions(obj, monitor_resolution, monitor_dimensions_mm, monkey_distance_mm)
            assert(isvector(monitor_resolution) && length(monitor_resolution) == 2)
            monitor_resolution = monitor_resolution(:)';
            assert(isvector(monitor_dimensions_mm) && length(monitor_dimensions_mm) == 2)
            monitor_dimensions_mm = monitor_dimensions_mm(:)';
            
            obj.monitor_specs = struct();
            obj.monitor_specs.pixels = monitor_resolution;
            obj.monitor_specs.mm = monitor_dimensions_mm;
            obj.monitor_specs.mm_per_pixel = monitor_dimensions_mm./monitor_resolution;
            
            %assumes that 0,0 is the origin (and round to nearest pixel)
            obj.monitor_specs.center_pixels = round((obj.monitor_specs.pixels - 1)./2);
            
            obj.monkey_distance_mm = monkey_distance_mm;
            
            obj.has_dimensions = true;
        end
        
        function reset_circle_fixation_points(obj)
            fprintf('reset_circle_fixation_points\n');
            
            %grid = [-1,1;0,1;1,1; -1,0;0,0;1,0; -1,-1;0,-1;1,-1];
            
            %point 1 is in the center
            obj.circle_fixation_points(1,1).angle_x = 0;
            obj.circle_fixation_points(1,1).angle_y = 0;
            obj.circle_fixation_points(1,1).raw_x = -100;
            obj.circle_fixation_points(1,1).raw_y = -100; 
            %obj.circle_fixation_points(1,1).circle_h = [];
            
            %points 2-9 start from the top and go clockwise
            degree_div = 360/8;
            for i = 2:9
                degree_loc = (i-2)*degree_div + obj.fixation_theta_deg;
                x = obj.fixation_r_deg*sin(degree_loc/180*pi);
                y = obj.fixation_r_deg*cos(degree_loc/180*pi);
                %fprintf('%i %f %f %f\n', i, degree_loc, x, y);
                obj.circle_fixation_points(i,1).angle_x = x;
                obj.circle_fixation_points(i,1).angle_y = y;
                obj.circle_fixation_points(i,1).raw_x = -100;
                obj.circle_fixation_points(i,1).raw_y = -100; 
                %obj.circle_fixation_points(i,1).circle_h = []; %do not erase handle
            end
            
            obj.redraw_circle_fixation_points;
        end
        
        function calculate_calibration(obj)
            %reset calibration
            obj.is_calibrated = false;
            obj.calibration.deg_to_raw = struct();
            obj.calibration.raw_to_deg = struct();
            
            if ~obj.are_there_enough_set_fixation_points
                fprintf('not enough points yet\n');
                return;
            end
            
            %get a temporary calibration
            [x_raw, y_raw, x_deg, y_deg] = obj.get_valid_fixation_points;
            [~, x_m, ~] = regression(x_deg, x_raw);
            [~, y_m, ~] = regression(y_deg, y_raw);
            
            try
                if ~isempty(x_m) && ~isempty(y_m)
                    y_ratio = y_m/x_m;
                end
            catch
            end
            
            if (y_ratio > 1.5) || (y_ratio < 0.5)
                fprintf(2, 'invalid calibration, y_ratio is: %f\n', y_ratio);
                return;
            end
            
            [~, obj.calibration.deg_to_raw.x_m, obj.calibration.deg_to_raw.x_b] = regression(x_deg, x_raw);
            [~, obj.calibration.deg_to_raw.y_m, obj.calibration.deg_to_raw.y_b] = regression(y_deg, y_raw);
            
            [~, obj.calibration.raw_to_deg.x_m, obj.calibration.raw_to_deg.x_b] = regression(x_raw, x_deg);
            [~, obj.calibration.raw_to_deg.y_m, obj.calibration.raw_to_deg.y_b] = regression(y_raw, y_deg);

            
            obj.is_calibrated = true;
            obj.calculate_y_ratio;
        end
        
        function update_fixation_conversion(obj, update_raw)
            %disp('update fixation dia (deg/raw)');
            if nargin < 2
                update_raw = false;
            end
            
            
            
            if ~isempty(fields(obj.calibration)) && ~isempty(fields(obj.calibration.deg_to_raw))
                if update_raw && ~isempty(obj.fixation_diameter.deg)
                    %fprintf('update raw\n');
                    average_m = mean([obj.calibration.deg_to_raw.x_m, obj.calibration.deg_to_raw.y_m]);
                    obj.fixation_diameter.raw = round(average_m*obj.fixation_diameter.deg, 0);
                    %obj.fixation_diameter.raw
                else
                    %fprintf('update deg\n');
                    average_m = mean([obj.calibration.raw_to_deg.x_m, obj.calibration.raw_to_deg.y_m]);
                    obj.fixation_diameter.deg = round(average_m*obj.fixation_diameter.raw, 2);
                    %obj.fixation_diameter.deg
                end
                    
            end
            
            if obj.is_control_panel_open
                button_h = findobj(obj.control_panel_h, 'tag','e_target_raw');
                button_h.String = num2str(obj.fixation_diameter.raw);

                button_h = findobj(obj.control_panel_h, 'tag','e_target_deg');
                if ~isempty(obj.fixation_diameter.deg)
                    button_h.String = num2str(obj.fixation_diameter.deg, '%.2f');
                else
                    button_h.String = 'unknown';
                end
            end
        end
        
    end
    
    
    %% regular plotting methods
    methods
        display_control_panel(obj);
        cp_plot_update(obj, history_struct, varargin);
        cp_close(obj, varargin);
        cp_button_callback(obj, calling_obj, action_data, varargin);
        cp_click_callback(obj, varargin);
        cp_update_settings(obj);
        function eye_tracking_update_fixation_circle(obj)
            %disp('eye_tracking_update_fixation_circle')
            %send the updated circle
            %obj.monitor.serial_transmit('{"command":"set_eye_fixation_center","center":[0,0,100]}');
            obj.draw_circle_on_plot(obj.selected_circle_h, obj.selected_center, obj.fixation_diameter.raw/2);
            
            for i = 1:9
                %obj.circle_fixation_points(i,1).circle_h
                center = [obj.circle_fixation_points(i,1).raw_x, obj.circle_fixation_points(i,1).raw_y];
                obj.draw_circle_on_plot(obj.circle_fixation_points(i,1).circle_h, center, obj.fixation_diameter.raw/2);
            end
            
            obj.redraw_circle_fixation_points;
            
%             function plot_h = create_degree_circle(axes_h, style, thickness, color)
%                 plot_h = line(axes_h, -10, -10);
%                 plot_h.LineStyle = style;
%                 plot_h.LineWidth = thickness;
%                 plot_h.Color = color;
%             end
            
        end
        
        cp_update_axes(obj, center, center_radius);
        
        set_calibration_buttons(obj, specific_button, set_on);
        
        function update_tail(obj, varargin)
            try
                update_control = varargin{1};
                obj.eye_scatter_tail_sec = update_control.Value;
                obj.eye_scatter_tail_sec
            catch
            end
        end
        
        
        
        function draw_circle_on_plot(obj, plot_h, center, radius)
            assert(isscalar(radius))
            assert(isvector(center) && length(center) == 2)
            %update the circle on the plot
            if ishandle(plot_h)
                if sum(center >= 0) == 2
                    plot_h.XData = uint16(round(radius*cos(obj.circle_thetas) + center(1)));
                    plot_h.YData = uint16(round(radius*sin(obj.circle_thetas) + center(2)));
                else
                    plot_h.XData = [];
                    plot_h.YData = [];
                end
                drawnow;
            else
                fprintf('draw circle: not a handle\n');
            end
        end
        
        
        function [x_raw, y_raw, x_deg, y_deg] = get_valid_fixation_points(obj)
            x_raw = [obj.circle_fixation_points(:,1).raw_x];
            y_raw = [obj.circle_fixation_points(:,1).raw_y];
            x_deg = [obj.circle_fixation_points(:,1).angle_x];
            y_deg = [obj.circle_fixation_points(:,1).angle_y];

            %remove impossible locations
            remove_locs_x = find((x_raw < obj.raw_min) | (x_raw > obj.raw_max));
            remove_locs_y = find((y_raw < obj.raw_min) | (y_raw > obj.raw_max));
            remove_locs = unique([remove_locs_x, remove_locs_y]);

            x_raw(remove_locs) = [];
            y_raw(remove_locs) = [];
            x_deg(remove_locs) = [];
            y_deg(remove_locs) = [];
        end
        
        function [yes_no] = are_there_enough_set_fixation_points(obj)
            x_raw = obj.get_valid_fixation_points;
            yes_no = length(x_raw) > 2;
        end
        
        function create_circle_fixation_handles(obj)
            fprintf('create_circle_fixation_handles\n');
            for i = 1:9
                obj.circle_fixation_points(i,1).circle_h = line(obj.current_axes_h, -10, -10);
                obj.circle_fixation_points(i,1).circle_h.LineStyle = '-';
                obj.circle_fixation_points(i,1).circle_h.LineWidth = 0.5;
                obj.circle_fixation_points(i,1).circle_h.Color = 'k';
            end
        end
        
        function redraw_circle_fixation_points(obj)
            if ~obj.is_control_panel_open
                return;
            end
            
            for i = 1:9
                x = obj.circle_fixation_points(i,1).raw_x;
                y = obj.circle_fixation_points(i,1).raw_y;
                center = [x,y];
                obj.draw_circle_on_plot(obj.circle_fixation_points(i,1).circle_h, center, obj.fixation_diameter.raw/2);
            end
        end
        
        function angles = draw_monitor_on_plot(obj)
            if ~obj.is_calibrated
                %fprintf('not enough points to draw\n');
                obj.monitor_shape_h.XData = [];
                obj.monitor_shape_h.YData = [];
                return;
            end
            
            if isempty(obj.get_monitor_angles)
                fprintf('no monitor set yet\n');
                return;
            end
            
            %fprintf('draw monitor\n');
            [angles] = obj.get_monitor_angles;
            
            angles(:,1) = obj.calibration.deg_to_raw.x_b + angles(:,1)*obj.calibration.deg_to_raw.x_m;
            angles(:,2) = obj.calibration.deg_to_raw.y_b + angles(:,2)*obj.calibration.deg_to_raw.y_m;
            angles = round(angles);
            angles(end+1,:) = angles(1,:);
            
            try
                obj.monitor_shape_h.XData = angles(:,1);
                obj.monitor_shape_h.YData = angles(:,2);
            catch
            end
        end
        
%         function start(obj)
%             begin the task
%             obj.is_calibrating = true;
%             
%             setup the session
%             obj.setup_psychtoolbox;
%             
%         end
%         
%         function stop(obj)
%             if ~obj.is_calibrating
%                 return;
%             end
%             
%             obj.ptb_stop;
%             obj.is_calibrating = false;
%         end

        function [angles] = get_monitor_angles(obj)
            angles1 = obj.get_angles_for_pixels([0,0]);
            angles2 = obj.get_angles_for_pixels([obj.monitor_specs.pixels(1),0]);
            angles3 = obj.get_angles_for_pixels(obj.monitor_specs.pixels);
            angles4 = obj.get_angles_for_pixels([0,obj.monitor_specs.pixels(2)]);
            angles = cat(1, angles1, angles2, angles3, angles4);
        end


        function [angles] = get_angles_for_pixels(obj, pixels)
            angles = [];
            %in pt coordinates
            if isempty(obj.monkey_distance_mm) || obj.monkey_distance_mm <= 0
                fprintf('monkey_distance_mm is invalid\n');
                return ;
            end
            
            if isempty(fieldnames(obj.monitor_specs))
                fprintf('monitor_specs are invalid\n');
                return;
            end
            assert(isvector(pixels) && length(pixels) == 2)
            pixels = pixels(:)';    %make sure horizontal
            
            %angles are just geometry, no need for calibration
            distance_px = pixels - obj.monitor_specs.center_pixels;
            distance_mm = distance_px.*obj.monitor_specs.mm_per_pixel;
            angles = atan(distance_mm./obj.monkey_distance_mm)/pi*180;
        end
        
        function [pixels] = get_pixels_for_angles(obj, angles_in_degrees)
            %in pt coordinates
            if isempty(obj.monkey_distance_mm) || obj.monkey_distance_mm <= 0
                fprintf('monkey_distance_mm is invalid\n');
                return;
            end
            
            if isempty(fieldnames(obj.monitor_specs))
                fprintf('monitor_specs are invalid\n');
                return;
            end
            assert(isvector(angles_in_degrees) && length(angles_in_degrees) == 2)
            angles_in_degrees = angles_in_degrees(:)';    %make sure horizontal
            
            %angles are just geometry, no need for calibration
            distance_mm = tan(angles_in_degrees/180*pi).*obj.monkey_distance_mm;
            distance_px = distance_mm./obj.monitor_specs.mm_per_pixel;
            pixels = distance_px + obj.monitor_specs.center_pixels;
        end
        



    end
    
    %% methods for communication with experimental_monitor
    methods
        
%         function set_reward_channel(obj, reward_channel)
%             if ~obj.em.is_connected
%                 fprintf(2, 'matlab error: em must be connected before setting reward channel\n');
%                 return;
%             end
%             digital_out_count = obj.em.db_configuration.digital_out_count;
%             assert(reward_channel < digital_out_count, 'reward channel is too high');
%             assert(reward_channel >= 0, 'reward channel cannot be negative');
%             
%             obj.reward_channel = reward_channel;
%         end
        
        function set_history_length(obj, seconds, force)
            assert((seconds >= 0) && (seconds <=4), 'seconds must be between 0 and 4');
            if nargin < 3
                force = false;
            end
            
            if (~obj.channels_set) && ~force
                fprintf('channels are not set yet\n');
                return;
            end
            
            obj.history_seconds = seconds;
            
            %disable first
            obj.em.send_command("set_input_settings", "input_number", obj.inputs.x.number, "history_enabled", false);
            obj.em.send_command("set_input_settings", "input_number", obj.inputs.y.number, "history_enabled", false);
            
            if seconds > 0    
                fprintf('enabling history for eye channels\n');
                next_pow_of_2 = 2^ceil(log2(1000*seconds));
                
                obj.em.send_command("set_input_settings", "input_number", obj.inputs.x.number, ... 
                    "history_enabled", true, "history_length", next_pow_of_2);
                obj.em.send_command("set_input_settings", "input_number", obj.inputs.y.number, ... 
                    "history_enabled", true, "history_length", next_pow_of_2);
            end
        end

        function set_io_channels(obj, x_number, y_number, reward_number)
            if ~obj.em.is_connected
                fprintf(2, 'matlab error: em must be connected before setting io channels\n');
                return;
            end
            if nargin < 3
                fprintf(2, 'matlab_error: an x and y number must be provided\n');
                return;
            end
            
            digital_in_count = obj.em.db_configuration.digital_in_count;
            analog_in_count = obj.em.db_configuration.analog_in_count;
            assert(x_number ~= y_number, 'x and y cannot be the same')
            assert(x_number >= digital_in_count, 'x number is not analog');
            assert(x_number < (digital_in_count + analog_in_count), 'x number is too high');
            assert(y_number >= digital_in_count, 'y number is not analog');
            assert(y_number < (digital_in_count + analog_in_count), 'y number is too high');
            
            obj.inputs = struct();
            obj.inputs.x.number = x_number;
            obj.inputs.x.input_string = ['input_', num2str(x_number,'%02d')];
            obj.inputs.y.number = y_number;
            obj.inputs.y.input_string = ['input_', num2str(y_number,'%02d')];
            
            %if streaming, then stop
            if obj.is_streaming
                obj.stream_eye(false);
            end
            
            obj.refresh_callbacks;
            
            %set the desired settings
            %for x channel send messages to computer for all transitions
            obj.em.send_command("set_input_settings", "input_number", x_number, ... 
                "all_transitions", true, "send_to_computer", true, ...
                "target_met_min_tics", 100, "target_left_min_tics", 0);
            obj.em.send_command("set_input_settings", "input_number", y_number, ... 
                "all_transitions", false, "send_to_computer", false, ...
                "target_met_min_tics", 100, "target_left_min_tics", 0);
            
            %set up parent child relationship
            %and reset types and distances
            obj.em.send_command("set_input_settings", "input_number", y_number, ...
                "target_type", "circular_distance", "target_value", 0, ...
                "target_distance", 1);
            
            obj.em.send_command("set_input_settings", "input_number", x_number, ...
                "target_type", "circular_distance", "target_value", 0, ...
                "target_distance", 1, ...
                "enable_child", true, "child_number", y_number);
            
            %set a selection location
            obj.selected_center = [0, 0];
            obj.set_target(obj.selected_center(1), obj.selected_center(2), obj.fixation_diameter.raw/2, true)
            
            obj.set_history_length(1, true);  %default to 1 second
            
            %if a reward channel was provided
            if nargin > 3
                digital_out_count = obj.em.db_configuration.digital_out_count;
                assert(reward_number < digital_out_count, 'reward channel is too high');
                assert(reward_number >= 0, 'reward channel cannot be negative');
                
                fprintf('setting reward_channel\n');
                obj.reward_channel = reward_number;
                
                %set up x (parent) to set the ouput (but off for now)
                obj.set_autoreward(false);
                
%                 obj.em.send_command("set_output_settings", "output_number", reward_number, ...
%                     "on_value", on_value, "off_value", off_value, "set_value", off_value, ...
%                     "on_tics", obj.reward_on_tics, "off_tics", obj.reward_on_tics, "is_continuous", true, "enabled", false);
            end
            
            obj.channels_set = true;
            fprintf('done\n');
        end
        
        function set_autoreward(obj, enable)
            if (obj.reward_channel == -1)
                fprintf(2, 'reward channel not set up yet\n');
                return;
            end
            fprintf('set autoreward: %i\n', enable);
            
            if obj.reward_output_is_reversed
                on_value = 0;
                off_value = 1;
            else
                on_value = 1;
                off_value = 0;
            end
            
            %reset the reward type
            obj.em.send_command("set_output_settings", "output_number", obj.reward_channel, ...
                "on_value", on_value, "off_value", off_value, "set_value", off_value, ...
                "on_tics", obj.reward_on_tics, "off_tics", obj.reward_off_tics, "is_continuous", true, "enabled", false);
            
            %reset the output for the x input
            obj.em.send_command("set_input_settings", "input_number", obj.inputs.x.number, ...
            "output_enabled", enable, "output_number", obj.reward_channel);
        
            %if enable, then reset the target
            if enable
                obj.em.send_command("set_input_settings","input_number", obj.inputs.x.number,"reset_target",true)
            end
        end
        
        function success = check_callbacks(obj)
            callback_id_list = [obj.em.callbacks.callback_id];
            success = true;
            
            if isempty(find(callback_id_list == obj.x_callback_id, 1))
                success = false;
                fprintf(2, 'eyem x callback is not set in the em\n');
            end
            
            if isempty(find(callback_id_list == obj.y_callback_id, 1))
                success = false;
                fprintf(2, 'eyem y callback is not set in the em\n');
            end
        end
        
        function set_target(obj, raw_x, raw_y, raw_distance, force)
            if ~obj.em.is_connected
                fprintf(2, 'matlab error: em must be connected before setting io channels\n');
                return;
            end
            
            if nargin < 5
                force = false;
            end
            
            if (~obj.channels_set) && ~force
                fprintf(2, 'channels are not set yet\n');
                return;
            end
            
            %make sure they are integers
            raw_x = round(raw_x);
            raw_y = round(raw_y);
            raw_distance = round(raw_distance);
            
            if (raw_x < 0 || raw_x > 65535 || raw_y < 0 || raw_y > 65535)
                fprintf(2, 'invalid target values');
                return;
            end
            
            if (raw_distance < 0 || raw_distance > 65535)
                fprintf(2, 'invalid distance');
                return;
            end
            
            %y will be set up as the child of x
            %therefore must be set first
            obj.em.send_command("set_input_settings", "input_number", obj.inputs.y.number, ...
                "target_type", "circular_distance", "target_value", raw_y, ...
                "target_distance", raw_distance);
            
            obj.em.send_command("set_input_settings", "input_number", obj.inputs.x.number, ...
                "target_type", "circular_distance", "target_value", raw_x, ...
                "target_distance", raw_distance);
            
        end
        
        function enable_fixation_point(obj, fixation_number)
            if (fixation_number < 1) || (fixation_number > size(obj.circle_fixation_points, 1))
                fprintf('invalid fixation_number\n');
                return;
            end
            
            %TODO: send message to em to enable this fixation point
            
        end
        
        function y_ratio = calculate_y_ratio(obj)
            y_ratio = 0;
            
            try
                if ~isempty(obj.calibration.deg_to_raw.x_m) && ~isempty(obj.calibration.deg_to_raw.y_m)
                    y_ratio = obj.calibration.deg_to_raw.y_m/obj.calibration.deg_to_raw.x_m;
                end
            catch
            end
            
            if obj.is_control_panel_open
                button_h = findobj(obj.control_panel_h, 'tag','e_ratio');
                if y_ratio == 0
                    button_h.String = 'unknown';
                else
                    button_h.String = ['1:', num2str(y_ratio, '%.4f')];
                end
            end
        end
        
        
        
        function [yes] = are_points_inside_target(obj, target, points_array, threshold)
            if nargin < 4
                threshold = 1;
            end
            assert((threshold >= 0) && (threshold <= 1), 'threshold must be between 0 and 1');
            yes = false;    %default to no
            
            
            
        end
        
        
%         function get_eye_history(obj)
%             if ~obj.channels_set
%                 fprintf('channels have not been set yet\n');
%                 return;
%             end
%             
%             obj.em.send_command("get_input_history", "input_numbers", [obj.inputs.x.number, obj.inputs.y.number]);
%         end
        
        
        
%         function eye_callback(obj, input_struct, varargin)
% %             fprintf('eye_callback\n');
%             
%             if isfield(input_struct, obj.inputs.x.input_string)
%                 %fprintf('x\n');
%                 temp_struct = getfield(input_struct, obj.inputs.x.input_string); %#ok<GFLD>
%                 
%                 if isfield(temp_struct, 'history')
%                     obj.x_values = extract_new32_array(temp_struct.history);
%                 end
%                 
%             elseif isfield(input_struct, obj.inputs.y.input_string)
%                 %fprintf('y\n');
%                 temp_struct = getfield(input_struct, obj.inputs.y.input_string); %#ok<GFLD>
%                 
%                 if isfield(temp_struct, 'history')
%                     obj.y_values = extract_new32_array(temp_struct.history);
%                 end
%             end
%         end
        
        
       function [yes] = is_control_panel_open(obj)
            yes = false;
            try
                %disp('try')
                if ishandle(obj.control_panel_h)
                    %disp('yes check')
                    yes = true;
                end
            catch
                %disp('catch')
            end 
        end 
       
        
    end
    
    %other mehods
    methods
        function delete(obj, silent)
            if nargin < 2
                silent = false;
            end
            
            %close beforehand
            obj.cp_close;
            
            %remove from workspace
            base_variables = evalin('base', 'whos');
            for i = 1:length(base_variables)
                %test for eye_module
                if strcmp(base_variables(i).class, 'eye_module')
                    try
                        evalin('base', cat(2, 'clear ', base_variables(i).name));
                    catch
                    end
                end
            end
            
            try
                rmappdata(0, eye_module.app_data_name);
            catch
            end
            
            if ~silent
                fprintf('deleted\n');
            end
        end 
    end
    
    %% psychtoolbox properties and methods
    properties
        ptb_window_ptr
        ptb_window_rect
        ptb_monitor_number
        ptb_background_color
        ptb_black_color
        ptb_fixation_color = [255, 255, 255];
        ptb_resolution
        ptb_was_running_before_cp = false;
    end
    
    methods
        function yes_no = ptb_is_active(obj) %#ok<MANU>
            try
                yes_no = ~isempty(Screen('Windows'));
            catch
                yes_no = false;
            end
        end
        
        function ptb_start(obj, monitor_number)
            
            if ~obj.has_dimensions
                fprintf('does not yet have the necessary specs, exiting\n');
                return;
            end
            
            if ~obj.em.is_connected
                fprintf('em is not connected yet, exiting\n');
                return;
            end
            
            %test if any psychtoolbox windows are active
            window_pts = Screen('Windows');
            
            if obj.ptb_is_active
                %test all for main windows, and remove any that are not
                for i = 1:size(window_pts,1)
                    if Screen(window_pts(i), 'WindowKind') ~= 1
                        window_pts(i) = [];
                    end
                end
                
                if isempty(window_pts)
                    fprintf('no main pyschtoolbox window is active, but others are, unsure what to do so exiting\n');
                    return;
                end
                
                if size(window_pts, 1) > 1
                    fprintf('more than one main pyschtoolbox window is active, exiting\n');
                    return;
                end
                
                %no perfect way to test if a window is a on specific
                %monitor, so just compare the resolution
                [x_res, y_res] = Screen('WindowSize', window_pts);
                temp_ptb_resolution = [x_res, y_res];
                
                %compare resolution with monitor specs
                if sum(temp_ptb_resolution == obj.monitor_specs.pixels) ~= 2
                    fprintf('main psychtoolbox window resolution does not match monitor specs\n');
                    return;
                end
                
                %if good, then set resolution and window
                fprintf('grabbed the active pyschtoolbox window\n');
                obj.ptb_resolution = temp_ptb_resolution;
                obj.ptb_window_ptr = window_pts;
                
                return;
            end
            
            
            try
                %close all screens
                sca;

                %because psychtoolbox is finiky
                %and precise stimulus timing is unnecessary for calibration
                Screen('Preference', 'VisualDebugLevel', 0);
                Screen('Preference', 'SkipSyncTests', 1);
                
                if length(Screen('Screens')) < 2
                    fprintf('it is not recommended to run this with only one monitor\n');
                    return;
                end

                if nargin < 2
                    monitor_number = max(Screen('Screens'));
                else
                    if monitor_number > max(Screen('Screens'))
                        fprinf('invalid monitor number\n');
                        return;
                    end
                    
                    if monitor_number == 0
                        fprintf('it is not recommended to run psychtoolbox on the main monitor\n');
                        return;
                    end
                end
                obj.ptb_monitor_number = monitor_number;

                screen_props = Screen('Resolution', obj.ptb_monitor_number);
                obj.ptb_resolution = [screen_props.width, screen_props.height];

                %compare resolution with monitor specs
                if sum(obj.ptb_resolution == obj.monitor_specs.pixels) ~= 2
                    fprintf('monitor resolution does not match monitor specs\n');
                    return;
                end
                
                %opens the window
                [obj.ptb_window_ptr, obj.ptb_window_rect] = Screen('OpenWindow', obj.ptb_monitor_number, 0, [], [], 2);
                
                %get the colors
                obj.ptb_background_color = GrayIndex(obj.ptb_monitor_number); 
                obj.ptb_black_color = BlackIndex(obj.ptb_monitor_number); 

                %just draw blank screen
                obj.ptb_draw_blank_screen;            
                
            %everything above was in a try
            catch
                %if anything failed, then kill psychtoolbox and throw error
                obj.ptb_stop;
                fprintf(2, 'error with initializing psychtoolbox\n');
            end

        end
        
%         function calculate_desired_fixation_points(obj, deg_grid)
%             %assumes a 9 point grid
%             obj.list_of_fixation_points_deg = deg_grid*[0,0;-1,1;1,-1;0,1;0,-1;1,1;-1,-1;1,0;-1,0];
%         end
%         
%         function draw_fixation_point_number(obj, number)
%             assert(number >= 1 && number <= 9)
%             point = obj.list_of_fixation_points_deg(number, :);
%             obj.draw_fixation_point(point(1), point(2));
%         end

        function ptb_pursuit_calibration(obj)
%             end_points = [0,0];
%             end_points(end+1,:) = [5,0];
%             end_points(end+1,:) = [5,-5];
%             end_points(end+1,:) = [-5,-5];
%             end_points(end+1,:) = [-5,5];
%             end_points(end+1,:) = [5,5];
%             end_points(end+1,:) = [5,-10];
%             end_points(end+1,:) = [-10,-10];
%             end_points(end+1,:) = [-10,10];
%             end_points(end+1,:) = [10,10];
%             end_points(end+1,:) = [10,-10];
%             end_points(end+1,:) = [0,-10];
%             end_points(end+1,:) = [0,0];
            end_points = obj.get_monitor_angles;
            end_points(end+1,:) = end_points(1,:);  %copy the beginning to the end

            %subtract the circle radius
            remove_radius = obj.screen_fixation_diameter_deg/2 + 0.1;
            for i = 1:2
                end_points(end_points(:,i) < 0, i) = end_points(end_points(:,i) < 0, i) + remove_radius;
            end
            for i = 1:2
                end_points(end_points(:,i) > 0, i) = end_points(end_points(:,i) > 0, i) - remove_radius;
            end
            
            
            deltas_deg = sqrt((end_points(1:(end-1),1)-end_points(2:end,1)).^2 + (end_points(1:(end-1),2)-end_points(2:end,2)).^2);
            speeds_sec = deltas_deg/20;    %convert to seconds (with a constant)
            %pause_between_steps = 0.2;
            
            x_list = [];
            y_list = [];
            
            for step = 1:(size(end_points,1) - 1)
                [x_list_app, y_list_app] = obj.ptb_smooth_transition(end_points(step, :), end_points(step+1, :), speeds_sec(step));

                x_list = cat(1, x_list, x_list_app);
                y_list = cat(1, y_list, y_list_app);
            end
            obj.ptb_draw_x_y_list(x_list, y_list, 0, 0.02, true);
        end
        
        function [x_list, y_list] = ptb_smooth_transition(obj, start_pt, end_pt, speed_sec)
            
            distance = sqrt((start_pt(1,1)-end_pt(1,1)).^2 + (start_pt(1,2)-end_pt(1,2)).^2);
            division_deg = (obj.screen_fixation_diameter_deg); %move 1 radius
            divisions = distance/division_deg;
            %delay_seconds = speed_sec/(divisions+1);
            
            i_list = (0:divisions)';
            x_list = start_pt(1,1) + (end_pt(1,1) - start_pt(1,1))*i_list/divisions;
            y_list = start_pt(1,2) + (end_pt(1,2) - start_pt(1,2))*i_list/divisions;
            %obj.ptb_draw_x_y_list(x_list, y_list, 1, delay_seconds);
        end
        
        function ptb_draw_x_y_list(obj, x_list, y_list, current_i, delay_seconds, repeat)
            assert(iscolumn(x_list) && iscolumn(y_list))
            assert(length(x_list) == length(y_list))
            if nargin < 6
                repeat = false;
            end
            
            if (current_i < 0)
                return;
            end
            
            current_i = current_i + 1;
            
            if (current_i > length(x_list))
                if obj.stop_repeat
                    disp('stopping repeat');
                    return;
                end
                
                if repeat
                    disp('repeating');
                    current_i = 1;
                else
                    disp('end loop');
                    return;
                end
            end
                
            
            x = x_list(current_i);
            y = y_list(current_i);
            obj.ptb_draw_circle_deg(x, y);
            delay_call(delay_seconds, @obj.ptb_draw_x_y_list, x_list, y_list, current_i, delay_seconds, repeat);
        end
        
        
        function ptb_draw_grid_fixation_point(obj, fixation_number)
            if nargin < 2 || isempty(fixation_number)
                fprintf('fixation_number is required\n');
                return;
            end
            
            if (fixation_number < 1) || (fixation_number > size(obj.circle_fixation_points, 1))
                fprintf('invalid fixation_number\n');
                return;
            end
            
            x_deg = obj.circle_fixation_points(fixation_number, 1).angle_x;   
            y_deg = obj.circle_fixation_points(fixation_number, 1).angle_y;
            dia_deg = obj.screen_fixation_diameter_deg;
%             else
%                 if nargin < 3
%                     fprintf('not yet calibrated, need dia_px\n');
%                     return;
%                 end
%                 dia_deg = dia_px;
%             end
                
            obj.ptb_draw_circle_deg(x_deg, y_deg, dia_deg);
        end

        function ptb_draw_circle_deg(obj, x_deg, y_deg, dia_deg, color)
            %assumes x and y are in normal directions from center
            if ~obj.ptb_is_active
                fprintf('ptb is not active\n');
                return;
            end
            if nargin < 5
                if ~(isvector(obj.ptb_fixation_color) && length(obj.ptb_fixation_color) == 3)
                    fprintf('color not provided and fixation_color not valid\n');
                    return;
                end
                color = obj.ptb_fixation_color;
                
                if nargin < 4
                    dia_deg = obj.screen_fixation_diameter_deg;
                end
            end
            
            %temp pixels location (could be fractional)
            pixels = obj.get_pixels_for_angles([x_deg, y_deg]) - obj.monitor_specs.center_pixels;

            %calculate the dia_px (could be fractional)
            %using the center to the outer edge (compensates for distance from center)
            if sign(x_deg) == 0
                x_deg_offset = x_deg + abs(dia_deg)/2; %always point towards (0,0)
            else
                x_deg_offset = x_deg - sign(x_deg)*abs(dia_deg)/2; %always point towards (0,0)
            end
            
            radius = obj.get_pixels_for_angles([x_deg_offset, y_deg]) - obj.monitor_specs.center_pixels;
            dia_px = 2*abs(radius(1) - pixels(1));

            %flip y direction
            pixels(2) = -pixels(2);

            %store the location
%           obj.current_fixation_point_px = pixels;

            %draw the circle at the location
            obj.ptb_draw_circle_px(pixels(1), pixels(2), dia_px, color);
        end
        
        function ptb_draw_circle_px(obj, x_px, y_px, dia_px, color)
            % and (0,0) is the center of the screen
            %  x+ is moving right
            %  y+ is moving up
            % color is [r, g, b]
            
            if ~obj.ptb_is_active
                fprintf('ptb is not active\n');
                return;
            end
            if nargin < 5
                if ~(isvector(obj.ptb_fixation_color) && length(obj.ptb_fixation_color) == 3)
                    fprintf('color not provided and fixation_color not valid\n');
                    return;
                end
                color = obj.ptb_fixation_color;
            end
            
            assert (dia_px > 0, 'diameter must be larger than 0');
            
            %simplicity, round to nearest
            dia_px = round(dia_px);
            x_px = round(x_px);
            y_px = round(y_px);
            
            pixels = obj.monitor_specs.center_pixels;
            x_px = pixels(1) + x_px;
            y_px = pixels(2) + y_px;
            
            Screen('FillRect', obj.ptb_window_ptr, obj.ptb_background_color);
            location = CenterRectOnPoint([0, 0, dia_px, dia_px], x_px, y_px);
            Screen('FillOval', obj.ptb_window_ptr, color, location);
            Screen('Flip', obj.ptb_window_ptr);
        end
        
        function ptb_cycle_circle(obj, pauses)
            if nargin < 2
                pauses = 0.01;
            end
            
            delta = 0.1;
            for i = 0:delta:25
                obj.ptb_draw_circle_deg(i, 0, 0.5);
                pause(pauses);  %FIXME: remove
            end
            j = i - delta;
            for i = j:-delta:0
                obj.ptb_draw_circle_deg(i, 0, 0.5);
                pause(pauses);  %FIXME: remove
            end
        end
        
        function ptb_draw_blank_screen(obj)
            if ~obj.ptb_is_active
                return;
            end
            
            Screen('FillRect', obj.ptb_window_ptr, obj.ptb_background_color);
            Screen('Flip', obj.ptb_window_ptr);
        end
        
        function ptb_stop(obj) %#ok<MANU>
            %if any errors, just exit
            sca;
        end
        
        
    end
        
        
        %% old below here
        
            %unknown if useful, yet
    %properties
%         

%         fps_alpha = 0.9;    %weight for previous framerate
%         

%         eye_char_buffer = ''
%         
%         adc_min_value = 0;
%         adc_max_value = obj.raw_max;
    %end
%         function example_setup(obj)
%            obj.em.send_command("set_input_settings", "input_number", 8, "enable_history", true, "history_length", 512);
%            obj.em.send_command("set_input_settings", "input_number", 9, "enable_history", true, "history_length", 512);
%             
%         end
        
%         function set_fixation(obj, x_center, y_center, radius, led_on)
%             %update values
%             obj.eye_centear = [x_center, y_center, radius];
%             %send command
%             %obj.send_command(obj.commands.set_eye_fixation, strcat('[',num2str(x_center),',',num2str(y_center),',',num2str(radius),',',num2str(led_on),']'));
%         end
%         
%         function set_eye_analog_inputs(obj, x_in, y_in)
%             %obj.send_command(obj.commands.set_analog_inputs, strcat('[',num2str(x_in),',',num2str(y_in),']'));
%         end
        

         

        
%         
        
%         function eye_tracking_get_history(obj, varargin)
%             disp("should not be called");
%             %obj.em.send_command("get_input_history", "input_numbers", [8,9]);
%             %obj.monitor.send_command(obj.monitor.commands.get_eye_history);
%         end
    
    
    %% hidden properties and methods (not necessary for normal users)
    
    %used for the singletone storage
    properties (Constant)
        app_data_name = 'eye_singleton';
    end
    
    properties (Hidden)
        channels_set = false;
    end
    
    methods (Access = private)
        function obj = eye_module  
            obj.has_dimensions = false;
            obj.reset_circle_fixation_points;
            obj.array_of_fixation_circles(9,1).handle = '';
            
            obj.monitor_specs = struct();
            obj.monitor_specs.pixels = [0,0];
            obj.monitor_specs.mm = [0,0];
            obj.monitor_specs.mm_per_pixel =[0,0];
            
            obj.inputs = struct();
            obj.inputs.x.number = -1;
            obj.inputs.y.number = -1;
            obj.reward_channel = -1;
            
            obj.calibration.deg_to_raw = struct();
            obj.calibration.raw_to_deg = struct();
        end
        
        
        function set_button_on(obj, button_h, set_on) %#ok<INUSL>
            %on makes button dark, %off takes button back to "normal"
            if set_on
                button_h.Value = 1;
                button_h.BackgroundColor = [0.5, 0.5, 0.5];
            else
                button_h.Value = 0;
                button_h.BackgroundColor = [0.94, 0.94, 0.94];
            end
            
        end
        
        function clear_eye_cloud(obj,varargin)
            if obj.is_control_panel_open
                obj.eye_scatter_h.XData = -10;
                obj.eye_scatter_h.YData = -10;
                drawnow;
            end
        end
        
        
    end
end
