function [] = cp_update_settings(obj)

if ~ishandle(obj.control_panel_h)
    fprintf('control_panel is not open\n');
    return;
end

figure_h = obj.control_panel_h;

%% copy values
button_h = findobj(figure_h, 'tag','e_monitor_x_px');
button_h.String = num2str(obj.monitor_specs.pixels(1));

button_h = findobj(figure_h, 'tag','e_monitor_y_px');
button_h.String = num2str(obj.monitor_specs.pixels(2));

button_h = findobj(figure_h, 'tag','e_monitor_x_mm');
button_h.String = num2str(obj.monitor_specs.mm(1));

button_h = findobj(figure_h, 'tag','e_monitor_y_mm');
button_h.String = num2str(obj.monitor_specs.mm(2));

button_h = findobj(figure_h, 'tag','e_monkey_mm');
button_h.String = num2str(obj.monkey_distance_mm);

button_h = findobj(figure_h, 'tag','e_channel_x');
button_h.String = num2str(obj.inputs.x.number);

button_h = findobj(figure_h, 'tag','e_channel_y');
button_h.String = num2str(obj.inputs.y.number);


button_h = findobj(figure_h, 'tag','e_channel_reward');
button_h.String = num2str(obj.reward_channel);

end
