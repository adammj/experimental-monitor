function [] = display_control_panel(obj)


if ~obj.has_dimensions
    obj.cp_close;
    fprintf('dimensions have not been set yet\n');
    return;
end

if ~obj.channels_set
    obj.cp_close;
    fprintf('io channels have not been set yet\n');
    return;
end

if ~obj.check_callbacks
    obj.cp_close;
    return;
end

if ~obj.em.is_connected
    fprintf('experimental_monitor is not connected\n');
    return;
end

if obj.is_control_panel_open
    fprintf('control_panel is already open\n');
    return;
end

    
%mark not ready and grab figure
obj.control_panel_ready = false;

try
    figure_h = hgload('control_panel.fig');
    if ~(isvalid(figure_h) && ishandle(figure_h))
        fprintf(2, 'matlab error: missing control panel\n');
        return;
    end
catch
    fprintf(2, 'matlab error: missing control panel\n');
    return;
end

%if redrawing the plot, go back to full zoom
obj.plot_center = [obj.raw_middle, obj.raw_middle];
obj.plot_radius = obj.raw_middle;

%create a default center for now in the lower-left
obj.selected_center = [0, 0];

%% setup figure
%store the figure
obj.control_panel_h = figure_h;
obj.cp_dims_cm = figure_h.Position(3:4);
obj.cp_scale = 1;

%update the callbacks and make sure exit works
figure_h.Name = 'eye_module control panel';
figure_h.WindowButtonDownFcn = @obj.cp_click_callback;
figure_h.CloseRequestFcn = @obj.cp_close;
figure_h.RendererMode = 'manual';
figure_h.Renderer = 'painters'; %faster on macOS (vs 'OpenGL')


%close button
button_h = findobj(figure_h, 'tag','b_exit');
button_h.Callback = @obj.cp_close;

%get axes from figure, keep a local copy
ax_main = findobj(figure_h, 'tag','ax_main');
axes(ax_main);  %set as current axes
obj.current_axes_h = ax_main; %store to object
ax_main.Clipping = 'on';

%create plots
obj.eye_scatter_h = line(ax_main, -10, -10);
obj.eye_scatter_h.Marker = '.';
obj.eye_scatter_h.MarkerSize = 8;
obj.eye_scatter_h.MarkerEdgeColor = 'red';
obj.eye_scatter_h.MarkerFaceColor = 'red';
obj.eye_scatter_h.LineStyle = 'none';

%prepare to create several plots
hold(ax_main, 'on')

%update the axes
obj.cp_update_axes;

%create circle plots
obj.selected_circle_h = line(ax_main, -10, -10);
obj.create_circle_fixation_handles;
obj.eye_tracking_update_fixation_circle;

% for i = 1:9
%     obj.circle_fixation_points(i,1).circle_h = create_degree_circle(ax_main, '-', 0.5, 'k');
% end
%obj.redraw_circle_fixation_points;
    
% obj.degree_circle1_h = create_degree_circle(ax_main, ':', 0.5, 'k');
% obj.degree_circle2_h = create_degree_circle(ax_main, ':', 0.5, 'k');
% obj.degree_circle3_h = create_degree_circle(ax_main, ':', 0.5, 'k');
% obj.degree_circle4_h = create_degree_circle(ax_main, ':', 0.5, 'k');
% obj.degree_circle5_h = create_degree_circle(ax_main, ':', 0.5, 'k');

%create the calibration plots
% obj.fixation_points_h = line(ax_main, -10,-10);
% obj.fixation_points_h.LineStyle = 'none';
% obj.fixation_points_h.Marker = '+';
% obj.fixation_points_h.MarkerSize = 8;


%monitor
obj.monitor_shape_h = line(ax_main, -10,-10);
obj.monitor_shape_h.Color = 'g';

%disable hold
hold(ax_main, 'off')



%grab all buttons, and update callbacks
%% stream
button_h = findobj(figure_h, 'tag','b_stream');
button_h.Callback = @obj.cp_button_callback;
button_h.Value = 0;
obj.stream_button_h = button_h;

%% figure
button_h = findobj(figure_h, 'tag','b_fig_grow');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_fig_shrink');
button_h.Callback = @obj.cp_button_callback;

%% move
button_h = findobj(figure_h, 'tag','b_up');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_down');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_left');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_right');
button_h.Callback = @obj.cp_button_callback;

%% plot zoom
button_h = findobj(figure_h, 'tag','b_zoom_out');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_zoom_in');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_zoom_full');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_zoom_circle');
button_h.Callback = @obj.cp_button_callback;

%% circle size
button_h = findobj(figure_h, 'tag','b_circle_larger');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_circle_smaller');
button_h.Callback = @obj.cp_button_callback;

%% tail
button_h = findobj(figure_h, 'tag','s_tail');
button_h.Callback = @obj.cp_button_callback;
button_h.Min = 0;
button_h.Max = 0.5;
button_h.Value = 0;
button_h.SliderStep = [0.05, 0.05];

%% ptb
button_h = findobj(figure_h, 'tag','b_ptb_active');
if ~obj.ptb_is_active
    button_h.String = 'PTB is off';
    obj.ptb_was_running_before_cp = false;
else
    button_h.String = 'PTB is active';
    button_h.BackgroundColor = [0.5, 0.5, 0.5];
    obj.ptb_was_running_before_cp = true;
end
button_h.Callback = @obj.cp_button_callback;

%% spacing
button_h = findobj(figure_h, 'tag','e_r_deg');
button_h.String = num2str(obj.fixation_r_deg);
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','e_theta_deg');
button_h.String = num2str(obj.fixation_theta_deg);
button_h.Callback = @obj.cp_button_callback;

%% set/clear points
button_h = findobj(figure_h, 'tag','b_set_point');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_clear_point');
button_h.Callback = @obj.cp_button_callback;
button_h = findobj(figure_h, 'tag','b_clear_pts');
button_h.Callback = @obj.cp_button_callback;
obj.cp_update_settings;


%% calibration buttons
for i = 1:9
    button_h = findobj(figure_h, 'tag', ['b_fix_', num2str(i)]);
    button_h.Value = 0;
    button_h.Callback = @obj.cp_button_callback;
end


obj.calculate_calibration;
obj.draw_monitor_on_plot;
obj.update_fixation_conversion;
button_h = findobj(figure_h, 'tag','e_target_raw');
button_h.String = num2str(obj.fixation_diameter.raw);
button_h.Callback = @obj.cp_button_callback;

button_h = findobj(figure_h, 'tag','e_target_deg');
if ~isempty(obj.fixation_diameter.deg)
    button_h.String = num2str(obj.fixation_diameter.deg, '%.2f');
else
    button_h.String = 'unknown';
end
button_h.Callback = @obj.cp_button_callback;


button_h = findobj(figure_h, 'tag','e_ptb_deg');
button_h.String = num2str(obj.screen_fixation_diameter_deg);


button_h = findobj(figure_h, 'tag','b_autoreward');
button_h.Callback = @obj.cp_button_callback;

%set to no calibration button
obj.set_calibration_buttons(0);

%draw everything now
drawnow
obj.control_panel_ready = true;

end

% function plot_h = create_degree_circle(axes_h, style, thickness, color)
%     plot_h = line(axes_h, -10, -10);
%     plot_h.LineStyle = style;
%     plot_h.LineWidth = thickness;
%     plot_h.Color = color;
% end
