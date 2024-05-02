function [] = update_serial_configuration(obj, config_struct, varargin)

if ~obj.is_connected
    fprintf('must be connected first\n');
    return;
end

if nargin < 2 || isempty(config_struct)
    %remove the callbacks first
    obj.remove_callback("serial_configuration", true);
    
    if nargin > 1 && isempty(config_struct)
        fprintf('error with config_struct\n');
        return;
    else
        fprintf('updating serial configuration... ');
        obj.serial_configuration = struct();
        obj.add_callback("serial_configuration", @obj.update_serial_configuration, [], true);
        obj.send_command(obj.commands.get_serial_configuration);
        return;
    end
end


if isfield(config_struct, 'serial_configuration')

    obj.serial_configuration = config_struct.serial_configuration;

    %update the max string length
    obj.max_serial_string_length = obj.serial_configuration.max_string_length;
    obj.max_json_tokens = obj.serial_configuration.max_json_tokens;

    fprintf('success\n');
else
    fprintf('failure\n');
end

%now, remove the callback
obj.remove_callback("serial_configuration", true);

end
