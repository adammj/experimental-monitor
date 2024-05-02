function send_command(obj, command, varargin)
%function to make it easier to send commands

if mod(nargin, 2) ~= 0
    fprintf("error: there must be one command and 0+ pairs of arguments\n");
    return;
end

if (~ischar(command) && ~isstring(command))
    fprintf("error: command must be a string\n");
	return;
end

%create structure
output = struct();
output.command = command;

%add each additional pair
for i = 1:2:(nargin-2)
    if (~ischar(varargin{i}) && ~isstring(varargin{i}))
        fprintf("error: every parameter name must be a string\n");
        return;
    end
    
    output = setfield(output, char(varargin{i}), varargin{i+1}); %#ok<SFLD>
end

%convert the struct into a json string and transmit
try
    json_string = jsonencode(output);
    
    obj.serial_transmit(json_string);
catch
    fprintf('error: was unable to encode into json\n');
end


end
