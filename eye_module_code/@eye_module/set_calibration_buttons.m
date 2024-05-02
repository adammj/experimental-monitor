function set_calibration_buttons(obj, specific_button, set_on)

if specific_button < 0 || specific_button > 9
    fprintf('cannot set callibration button for %i\n', specific_button);
    return;
end

obj.current_circle_fixation = specific_button;

for i = 1:9
    try
        button_h = findobj(obj.control_panel_h, 'tag', ['b_fix_', num2str(i)]);
    catch
        fprintf('error with calibration button %i\n', i);
        return;
    end
    
    if ~ishandle(button_h)
        fprintf('error with calibration button %i\n', i);
        return;
    end
   
    if i == specific_button
        if nargin > 2
            if set_on
                obj.set_button_on(button_h, true);
            else
                obj.set_button_on(button_h, false);
            end
        else
            obj.set_button_on(button_h, true);
        end
    else
        obj.set_button_on(button_h, false);
    end
end

if specific_button > 0
    %if there is a real raw, then set the selected circle to the point
    if obj.circle_fixation_points(specific_button).raw_x >= 0
        x = obj.circle_fixation_points(specific_button).raw_x;
        y = obj.circle_fixation_points(specific_button).raw_y;

        obj.selected_center = [x, y];
        obj.eye_tracking_update_fixation_circle;
    end
end

end

