function [] = update_ti_configuration(obj, config_struct, varargin)

if ~obj.is_connected
    fprintf('must be connected first\n');
    return;
end

if nargin < 2 || isempty(config_struct)
    fprintf('updating ti configuration... ');
    
    obj.remove_callback("ti_configuration", true);
    obj.ti_configuration = struct();
    
    obj.add_callback("ti_configuration", @obj.update_ti_configuration, [], true);
    obj.send_command(obj.commands.get_ti_configuration);
    return;
end


if isfield(config_struct, 'ti_configuration')

    obj.ti_configuration = config_struct.ti_configuration;
    obj.ti_configuration.hardware_id = num2str(obj.ti_configuration.hardware_id);
    
    fprintf('success\n');
else
    fprintf('failure\n');
end

%now, remove the callback
obj.remove_callback("ti_configuration", true);

end
