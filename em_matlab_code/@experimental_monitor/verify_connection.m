function [actually_connected] = verify_connection(obj, disconnect_if_not_connected)

if nargin < 2
    disconnect_if_not_connected = false;
end
actually_connected = false;

%if the serial connection is empty, it's not connected
if ~isempty(obj.serial_connection) && ~isempty(obj.java_serial_object)
    %test the connection
    try
        %test that port is open
        if strcmp(obj.serial_connection.Status, 'open')
            %and that the port is valid
            %just try to send a return, if it fails, it isn't connected
            fwrite(obj.java_serial_object, uint8(10), 1, 0, 0, 0);
            actually_connected = true;
        end
    catch
    end
end


%if not connected, and asked to disconnected, do so
if ~actually_connected && disconnect_if_not_connected
    fprintf(2, 'experimental_monitor is no longer connected\n');
    obj.disconnect(true);
end

%make sure to correct the is_connected flag
%only for disconnected, as a new connections requires verification
if ~actually_connected
    obj.is_connected = false;
end

end