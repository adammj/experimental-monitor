function cp_button_callback(obj, button_h, action_data, varargin) %#ok<INUSD>

if ~obj.control_panel_ready
    fprintf('control panel is not fully initialized yet\n');
    return;
end

if ~obj.is_control_panel_open
    return;
end

%only designed for buttons now
if ~isa(button_h, 'matlab.ui.control.UIControl')
    fprintf('only designed for button callbacks\n');
    return;
end

%get the tag
try
    tag = button_h.Tag;
    if isempty(tag)
        fprintf('Tag is empty\n');
        return;
    end
catch
    fprintf('Tag does not exist\n');
    return;
end

%% calibration buttons (test first)
if strcmp(tag(1:(end-1)), 'b_fix_')
    selected_pt = str2num(tag(end)); %#ok<ST2NM>
    
    if obj.circle_fixation_points(selected_pt).raw_x >= 0
        center = [obj.circle_fixation_points(selected_pt).raw_x, obj.circle_fixation_points(selected_pt).raw_y];
        obj.plot_center = center;
        obj.plot_radius = obj.fixation_diameter.raw*1.5;
        obj.cp_update_axes;
    else
        obj.plot_center = [obj.raw_middle, obj.raw_middle];
        obj.plot_radius = obj.raw_middle;
        obj.cp_update_axes;
    end
    
    obj.set_calibration_buttons(selected_pt);
    obj.ptb_draw_grid_fixation_point(selected_pt);
    return;
end

%go to appropriate action
switch (tag)
    %% stream on/off
    case 'b_stream'
        %start/stop recording
        if ~obj.em.is_connected
            fprintf('em must be connected\n');
            button_h.Value = 0;
            return;
        end
        
        if ~obj.is_streaming
            obj.set_button_on(button_h, true);
            obj.stream_eye(true);
        else
            obj.stream_eye(false);
            obj.set_button_on(button_h, false);
        end
        
    %% move the plot
    case 'b_up'
        obj.plot_center(2) = round(obj.plot_center(2) - obj.plot_radius/10,0);
        obj.cp_update_axes;
    case 'b_down'
        obj.plot_center(2) = round(obj.plot_center(2) + obj.plot_radius/10,0);
        obj.cp_update_axes;
    case 'b_right'
        obj.plot_center(1) = round(obj.plot_center(1) - obj.plot_radius/10,0);
        obj.cp_update_axes;
    case 'b_left'
        obj.plot_center(1) = round(obj.plot_center(1) + obj.plot_radius/10,0);
        obj.cp_update_axes;
        
    %% zoom the plot
    case 'b_zoom_out'
        obj.plot_radius = round(obj.plot_radius * 1.1,0);
        obj.cp_update_axes;
    case 'b_zoom_in'
        obj.plot_radius = round(obj.plot_radius / 1.1,0);
        obj.plot_radius = max(obj.plot_radius, 200);
        obj.cp_update_axes;
    case 'b_zoom_full'
        obj.plot_center = [obj.raw_middle, obj.raw_middle];
        obj.plot_radius = obj.raw_middle;
        obj.cp_update_axes;
    case 'b_zoom_circle'
        if sum(obj.selected_center < 0) > 0
            return;
        end
        
        obj.plot_center = obj.selected_center;
        obj.plot_radius = obj.fixation_diameter.raw*1.5;
        obj.cp_update_axes;

    %% shrink/grow the circle
    case 'b_circle_larger'
        if obj.calculate_y_ratio == 0
            obj.fixation_diameter.raw = round(obj.fixation_diameter.raw * 1.1, 0);
            obj.fixation_diameter.raw = min(obj.fixation_diameter.raw, obj.raw_middle);
            obj.update_fixation_conversion(false);
        else
            obj.fixation_diameter.deg = round(obj.fixation_diameter.deg + 0.1, 2);
            obj.update_fixation_conversion(true);
        end
        obj.eye_tracking_update_fixation_circle;
        
    case 'b_circle_smaller'
        if obj.calculate_y_ratio == 0
            obj.fixation_diameter.raw = round(obj.fixation_diameter.raw / 1.1, 0); 
            obj.fixation_diameter.raw = max(obj.fixation_diameter.raw, 160);
            obj.update_fixation_conversion(false);
        else
            obj.fixation_diameter.deg = round(obj.fixation_diameter.deg - 0.1, 2);
            obj.update_fixation_conversion(true);
        end
        obj.eye_tracking_update_fixation_circle;
        
    %% tail
    case 's_tail'
        obj.eye_scatter_tail_sec = button_h.Value;
        
    %% ptb active
    case 'b_ptb_active'
        
        if obj.ptb_is_active
            if ~obj.ptb_was_running_before_cp
                button_h.String = 'PTB is off';
                obj.set_button_on(button_h, false);
                obj.ptb_stop;
            else
                errordlg('PTB was active before the control panel was open. Close the control panel first');
            end
        else
            button_h.String = 'PTB is active';
            obj.set_button_on(button_h, true);
            obj.ptb_start;
        end
        
        
    case 'e_r_deg'
        temp_value = str2double(button_h.String);
        if temp_value <= 1 || temp_value > 25 || isnan(temp_value)
            errordlg('Spacing is probably invalid. Resetting.');
            button_h.String = num2str(obj.fixation_r_deg);
        else
            
            answer = questdlg('This will reset all grid fixation points. Are you sure?', 'Are you sure?', 'No', 'Yes', 'Yes');
            
            if strcmp(answer, 'Yes')
                obj.fixation_r_deg = temp_value;
                obj.reset_circle_fixation_points;
            else
                button_h.String = num2str(obj.fixation_r_deg);
            end
        end
        
    case 'e_theta_deg'
        temp_value = str2double(button_h.String);
        if temp_value < 0 || temp_value > 45 || isnan(temp_value)
            errordlg('Spacing is probably invalid. Resetting.');
            button_h.String = num2str(obj.fixation_r_deg);
        else
            
            answer = questdlg('This will reset all grid fixation points. Are you sure?', 'Are you sure?', 'No', 'Yes', 'Yes');
            
            if strcmp(answer, 'Yes')
                obj.fixation_theta_deg = temp_value;
                obj.reset_circle_fixation_points;
            else
                button_h.String = num2str(obj.fixation_r_deg);
            end
        end
        
    %% grow/shrink fig
    case 'b_fig_grow'
        obj.cp_scale = obj.cp_scale * 1.1; 
        temp_pos = obj.control_panel_h.Position;
        temp_pos(3:4) = obj.cp_dims_cm*obj.cp_scale;
        obj.control_panel_h.Position = temp_pos;
        
    case 'b_fig_shrink'
        obj.cp_scale = obj.cp_scale / 1.1;
        temp_pos = obj.control_panel_h.Position;
        temp_pos(3:4) = obj.cp_dims_cm*obj.cp_scale;
        obj.control_panel_h.Position = temp_pos;
        
    case 'b_clear_pts'
        obj.reset_circle_fixation_points;
        disp('eye_tracking_update_fixation_circle')
        obj.eye_tracking_update_fixation_circle;
        disp('redraw_circle_fixation_points')
        obj.redraw_circle_fixation_points;
        %recalculate and update drawings
        obj.calculate_calibration;
        obj.draw_monitor_on_plot;
        if obj.calculate_y_ratio == 0
            obj.update_fixation_conversion(false);
        else
            obj.update_fixation_conversion(true);
        end
        
    %% set/clear
    case 'b_set_point'
        if isempty(obj.current_circle_fixation) || obj.current_circle_fixation == 0
            fprintf('no point selected yet\n');
            return;
        end
        
        fprintf('set\n');
        temp_center = obj.selected_center;
        
        %store the location and redraw fixation points
        obj.circle_fixation_points(obj.current_circle_fixation).raw_x = temp_center(1);
        obj.circle_fixation_points(obj.current_circle_fixation).raw_y = temp_center(2);
        obj.redraw_circle_fixation_points;
        
        %recalculate and update drawings
        obj.calculate_calibration;
        obj.draw_monitor_on_plot;
        if obj.calculate_y_ratio == 0
            obj.update_fixation_conversion(false);
        else
            obj.update_fixation_conversion(true);
        end
        obj.eye_tracking_update_fixation_circle;
        
    case 'b_clear_point'
        fprintf('clear\n');
        obj.circle_fixation_points(obj.current_circle_fixation).raw_x = -10;
        obj.circle_fixation_points(obj.current_circle_fixation).raw_y = -10;
        obj.redraw_circle_fixation_points;
        
        %recalculate and update drawings
        obj.calculate_calibration;
        obj.draw_monitor_on_plot;
        if obj.calculate_y_ratio == 0
            obj.update_fixation_conversion(false);
        else
            obj.update_fixation_conversion(true);
        end
        obj.eye_tracking_update_fixation_circle;

        
    case 'e_target_deg'
        if obj.calculate_y_ratio ~= 0
            try
                temp_dia = round(str2double(button_h.String), 2);
                if temp_dia > 0 
                    obj.fixation_diameter.deg = temp_dia;
                    obj.update_fixation_conversion(true);
                    obj.eye_tracking_update_fixation_circle;
                else
                    button_h.String = num2str(obj.fixation_diameter.deg, '%.2f');
                end
            catch
            end
        else
            button_h.String = 'unknown';
        end
        
    case 'e_target_raw'
        try
            temp_dia = round(str2double(button_h.String), 0);
            if temp_dia > 0 
                obj.fixation_diameter.raw = temp_dia;
                obj.update_fixation_conversion(false);
                obj.eye_tracking_update_fixation_circle;
            else
                button_h.String = num2str(obj.fixation_diameter.raw);
            end
        catch
        end
        
        
    case 'b_autoreward'
        if button_h.Value == 1
            obj.set_autoreward(true);
        else
            obj.set_autoreward(false);
        end
            
    otherwise
        fprintf('unknown tag: %s\n', tag);
end
    
    
end

%function to clear the eye_scatter after


