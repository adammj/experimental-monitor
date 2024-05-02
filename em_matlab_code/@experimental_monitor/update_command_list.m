function update_command_list(obj, commands_struct, varargin)


if ~obj.is_connected
    fprintf('must be connected first\n');
    return;
end

if nargin < 2 || isempty(commands_struct)
    fprintf('updating commands... ');
    
    obj.remove_callback("commands", true);

    %erase commands and set list_command string
    obj.commands = struct();
    obj.commands = setfield(obj.commands, obj.list_command_string, obj.list_command_string); %#ok<SFLD>

    obj.add_callback("commands", @obj.update_command_list, [], true);
    obj.send_command(obj.commands.get_commands);
    return;
end

%only reaches here upon being called
if isfield(commands_struct, obj.list_commands_reply)
    
    %clear commands
    obj.commands = struct();
    
    %copy each of the commands to the struct
    for j = 1:length(commands_struct.commands)
        cmd_string = commands_struct.commands{j};
        obj.commands = setfield(obj.commands, cmd_string, cmd_string); %#ok<SFLD>
    end

    fprintf('success\n');
else
    fprintf('failure\n');
end

obj.remove_callback("commands", true);

end
