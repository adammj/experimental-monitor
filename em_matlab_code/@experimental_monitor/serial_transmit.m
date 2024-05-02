function serial_transmit(obj, output, alternative_connection)
%output must be one of the following:
%  1) char array
%  2) string
% trying to remove  3) struct, with command field


%use the current connection, or an alternative
if nargin < 3
    if ~obj.is_connected
        fprintf('not connected\n');
        return;
    end
end


%test if struct
if isstruct(output)
    fprintf('tried to send struct\n');
    struct_fields = fieldnames(output);
    command_location = strcmp(struct_fields,'command');
    
    %if no command given, exit
    if sum(command_location) == 0
        fprintf('no command field\n');
        return;
    end
    
    %if command isn't the first field, reorder and continue
    if command_location(1) == 0
        reorder_array = (1:length(command_location))';
        reorder_array(:,2) = command_location;
        reorder_array = sortrows(reorder_array, -2);
        output = orderfields(output, reorder_array(:,1));
    end
    
    %try to encode it in json
    try
        output = jsonencode(output);
    catch
        fprintf('error encoding struct into json, not transmitting\n');
        return;
    end
        
else 
    %otherwise, it must be a char array or string
    assert(ischar(output) || isstring(output))
    
    %check array direction
    if iscolumn(output)
        output = output';
    end
    
     %try to encode it in json
    try
        temp = jsondecode(output);
        if isempty(fieldnames(temp))
            fprintf('no valid json string, not transmitting\n');
            return;
        end
    catch
        fprintf('no valid json string, not transmitting\n');
        return;
    end 
end

%check that there are no LF or CR inside the string so far
if ~isempty(find(output == char(10), 1)) || ~isempty(find(output == char(13), 1)) %#ok<CHARTEN>
    fprintf('cannot have a linefeed or carriage return in output\n');
    return;
end


%add the linefeed to the end (triggers em to parse the command)
output(end+1) = char(10); %#ok<CHARTEN>
output = uint8(output);         %convert
output_len = length(output);    %get the length


%check length against what is possible
if length(output) > obj.max_serial_string_length
    fprintf('cannot transmit such a long string (%i) to em\n', length(output));
    fprintf('normal transmit: %s\n', output(1:(end-1)));
    return;
end

if nargin < 3
    %default to using the normal way
    %fprintf('normal transmit: %s\n', output(1:(end-1)));
    
    if obj.transmit_logging
        f_h = fopen('em_logging.txt','a');  %append to file (create if necessary)
        fprintf(f_h, '%s\n', datestr(now,'yyyy/mm/dd-HH:MM:SS.FFF')); %print a timestamp
        fprintf(f_h, '%s', output); %no final newline
        fclose(f_h);
    end
    
    %try to send directly, otherwise check if connected
    try
        fwrite(obj.java_serial_object, output, output_len, 0, 0, 0);
    catch
        fprintf(2, 'matlab error: error sending to em\n');
        obj.verify_connection(true);
    end
    obj.last_transmit_timestamp = datenummx(clock); %current_time;
    
else
    
    %otherwise, use the alternative connection
    java_serial_object = igetfield(alternative_connection, 'jobject');
    
    %send directly
    try
        fwrite(java_serial_object, output, output_len, 0, 0, 0); %#ok<GTARG>
    catch
        fprintf(2, 'matlab error: cannot transmit to monitor\n');
    end
end

end
