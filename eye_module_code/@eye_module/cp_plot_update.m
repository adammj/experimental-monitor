function cp_plot_update(obj, history_struct, varargin)

%keep a persistent eye_values_array
persistent eye_values_array partial_x_ts partial_y_ts partial_x_history partial_y_history 

%exit if not streaming
if ~obj.is_streaming
    eye_values_array = [];
    partial_x_ts = [];
    partial_x_history = [];
    partial_y_ts = [];
    partial_y_history = [];
    obj.clear_eye_cloud;
    return;
end

%extract x
if isfield(history_struct, obj.inputs.x.input_string)
    x_struct = history_struct.(obj.inputs.x.input_string);
    
    if ~strcmp(x_struct.reason, 'get')
        return;
    end
    
    if isfield(x_struct, 'history')
        partial_x_ts =  x_struct.timestamp;
        partial_x_history =  x_struct.history;
    end
end

%extract y
if isfield(history_struct, obj.inputs.y.input_string)
    y_struct = history_struct.(obj.inputs.y.input_string);
    
    if ~strcmp(y_struct.reason, 'get')
        return;
    end
    
    if isfield(y_struct, 'history')
        partial_y_ts =  y_struct.timestamp;
        partial_y_history =  y_struct.history;
    end
end

%if the partial timestamps don't match, return
if partial_x_ts ~= partial_y_ts
    return;
end


%if they match in length, then try to combine them
try
    temp_eye_values_array = [partial_x_history, partial_y_history];
catch
    fprintf(2, 'matlab error: mismatched lengths\n');
    return;
end

if size(temp_eye_values_array, 2) ~= 2
    return;
end

%try to append, or reset
try
    eye_values_array = cat(1, eye_values_array, temp_eye_values_array);
catch
    eye_values_array = temp_eye_values_array;
end

%if number array is valid
if ~isempty(eye_values_array)
    
    if obj.eye_scatter_tail_sec > 0
        scatter_length = 1000 * obj.eye_scatter_tail_sec;
        if size(eye_values_array, 1) > scatter_length
            %take the bottom rows
            eye_values_array = eye_values_array((end-scatter_length+1):end, :);
        end
    end

    %update plot data
    try
        %get the unique values to plot
        unique_values_array = unique(eye_values_array, 'rows');
        obj.eye_scatter_h.XData = unique_values_array(:,1);
        obj.eye_scatter_h.YData = unique_values_array(:,2);
        
        if obj.eye_scatter_tail_sec <= 0
            eye_values_array = [];  %reset after plotting
            obj.eye_scatter_tail_sec = 0;   %cannot be less than 0
        end
    catch
    end

    %limits update to 20 Hz
    drawnow limitrate;
end

end
