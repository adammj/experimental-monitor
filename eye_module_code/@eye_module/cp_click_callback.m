function cp_click_callback(obj, varargin)
    
if ~obj.control_panel_ready
    fprintf('control panel is not fully initialized yet\n');
    return;
end

    %get the click point
    pos = get(obj.control_panel_h.CurrentAxes, 'CurrentPoint');
    center_x = round(pos(1,1)); %round to nearest
    center_y = round(pos(2,2)); %round to nearest

    %don't allow clicks outside of the range
    if (center_x < obj.x_range(1)) || (center_x > obj.x_range(2)) || (center_y < obj.y_range(1)) || (center_y > obj.y_range(2))
        return;
    end
    
    %constrain the values
    center_x = max(center_x, obj.x_range(1));
    center_y = max(center_y, obj.y_range(1));
    center_x = min(center_x, obj.x_range(2));
    center_y = min(center_y, obj.y_range(2));

    %update values
    obj.selected_center(1) = center_x;
    obj.selected_center(2) = center_y;
    
    obj.set_target(center_x, center_y, obj.fixation_diameter.raw/2)
    
    %update the fixation circle
    obj.eye_tracking_update_fixation_circle;
end
