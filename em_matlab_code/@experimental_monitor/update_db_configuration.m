function [] = update_db_configuration(obj, config_struct, varargin)

if ~obj.is_connected
    fprintf('must be connected first\n');
    return;
end

if nargin < 2 || isempty(config_struct)
    %remove the callbacks first
    obj.remove_callback( "db_configuration", true);
    obj.db_configuration = struct();
    
    fprintf('updating db configuration... ');
        
    obj.add_callback("db_configuration", @obj.update_db_configuration, [], true);
    obj.send_command(obj.commands.get_daughterboard_configuration);
    return;
end


if isfield(config_struct, 'db_configuration')

    obj.db_configuration = config_struct.db_configuration;
    
    fprintf('success\n');
else
    fprintf('failure\n');
end

%now, remove the callback
obj.remove_callback( "db_configuration", true);

end
