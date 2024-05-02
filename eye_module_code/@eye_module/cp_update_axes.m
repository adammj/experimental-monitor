function cp_update_axes(obj, center, center_radius)
            
if nargin < 3
    center = obj.plot_center; 
    center_radius = obj.plot_radius;
end

%check inputs
assert(isvector(center))
assert(length(center) == 2)
assert(isscalar(center_radius))


%check max_dist
center_radius = max(center_radius, 50);

%if trying to set a larger radius, then reset center
if center_radius > obj.raw_middle
    center = [obj.raw_middle, obj.raw_middle];
end

center_radius = min(center_radius, obj.raw_middle);

%calculate the temporary values
temp_x_range = [center(1) - center_radius, center(1) + center_radius];
temp_y_range = [center(2) - center_radius, center(2) + center_radius];

%now, make sure that the ranges are equal

%limit the values to the max range (and shift to accomodate)
if temp_x_range(1) < 0
    temp_x_range(1) = 0;
    temp_x_range(2) = temp_x_range(1) + center_radius * 2;
end

if temp_x_range(2) > obj.raw_max
    temp_x_range(2) = obj.raw_max;
    temp_x_range(1) = temp_x_range(2) - center_radius * 2;
end

if temp_y_range(1) < 0
    temp_y_range(1) = 0;
    temp_y_range(2) = temp_y_range(1) + center_radius * 2;
end

if temp_y_range(2) > obj.raw_max
    temp_y_range(2) = obj.raw_max;
    temp_y_range(1) = temp_y_range(2) - center_radius * 2;
end

%update center
center(1) = (temp_x_range(1) + temp_x_range(2)) / 2;
center(2) = (temp_y_range(1) + temp_y_range(2)) / 2;

%store the values
obj.x_range = temp_x_range;
obj.y_range = temp_y_range;
obj.plot_radius = center_radius;
obj.plot_center = center;


%update the plot limits, ticks and labels
% xlim([temp_x_range(1), temp_x_range(2)]);  
% ylim([temp_y_range(1), temp_y_range(2)]);
obj.current_axes_h.XLim = [temp_x_range(1), temp_x_range(2)];
obj.current_axes_h.YLim = [temp_y_range(1), temp_y_range(2)];


%             x_div = diff(temp_x_range)/4;
%             y_div = diff(temp_y_range)/4;
%             obj.control_panel_h.CurrentAxes.XTick = temp_x_range(1):x_div:temp_x_range(2);
%             obj.control_panel_h.CurrentAxes.YTick = temp_y_range(1):y_div:temp_y_range(2);
% obj.control_panel_h.CurrentAxes.XTick = temp_x_range(1):diff(temp_x_range):temp_x_range(2);
% obj.control_panel_h.CurrentAxes.YTick = temp_y_range(1):diff(temp_y_range):temp_y_range(2);
%current_axes = findobj(obj.control_panel_h, 'tag', 'axes1')

voltage_range = diff(temp_x_range)/3200;

x_volts_values = [];
x_volts_labels = {};
y_volts_values = [];
y_volts_labels = {};
volt_i = 0;

%calculate a nice looking delta
volt_delta = 1;
if voltage_range < 5
    volt_delta = 0.5;
    if voltage_range < 2.5
        volt_delta = 0.25;
        if voltage_range < 1
            volt_delta = 0.1;
            if voltage_range < 0.5
                volt_delta = 0.05;
            end
        end
    end
end

for volts = -10:volt_delta:10
    volt_i = volt_i + 1;
    x_volts_values(volt_i) = volts*3200 + 32768; %#ok<AGROW>
    x_volts_labels{volt_i} = num2str(volts); %#ok<AGROW>
    y_volts_values(volt_i) = volts*3200 + 32768; %#ok<AGROW>
    y_volts_labels{volt_i} = num2str(volts); %#ok<AGROW>
end

%set x labels
below_i = find(x_volts_values < temp_x_range(1));
x_volts_values(below_i) = [];
x_volts_labels(below_i) = [];
above_i = find(x_volts_values > temp_x_range(2));
x_volts_values(above_i) = [];
x_volts_labels(above_i) = [];
obj.current_axes_h.XTick = x_volts_values;
obj.current_axes_h.XTickLabel = x_volts_labels;

%set y labels
below_i = find(y_volts_values < temp_y_range(1));
y_volts_values(below_i) = [];
y_volts_labels(below_i) = [];
above_i = find(y_volts_values > temp_y_range(2));
y_volts_values(above_i) = [];
y_volts_labels(above_i) = [];
obj.current_axes_h.YTick = y_volts_values;
obj.current_axes_h.YTickLabel = y_volts_labels;


% old label code
%obj.current_axes_h.XTick = 32768;
%obj.current_axes_h.YTick = 32768;
% obj.current_axes_h.XTick = temp_x_range(1):diff(temp_x_range):temp_x_range(2);
% obj.current_axes_h.YTick = temp_y_range(1):diff(temp_y_range):temp_y_range(2);

%put the ticks in terms of percent
% delta_x = diff(((temp_x_range)-obj.raw_middle)/327.68);
% delta_y = diff(((temp_y_range)-obj.raw_middle)/327.68);
% smallest_delta = min(delta_x, delta_y);
% digits = max((-round(log10(smallest_delta)) + 1), 0);
% label_format = ['%.',num2str(digits),'f%%'];

% obj.control_panel_h.CurrentAxes.XTickLabel = num2str(((temp_x_range')-obj.raw_middle)/327.68, label_format);
% obj.control_panel_h.CurrentAxes.YTickLabel = num2str(((temp_y_range')-obj.raw_middle)/327.68, label_format);
%obj.current_axes_h.XTickLabel = num2str(((temp_x_range')-obj.raw_middle)/327.68, label_format);
%obj.current_axes_h.YTickLabel = num2str(((temp_y_range')-obj.raw_middle)/327.68, label_format);


drawnow;

end
