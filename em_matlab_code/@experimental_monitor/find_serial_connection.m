function [success] = find_serial_connection(obj)
%find list of serial ports, and make changes for specific platforms
success = false;    %default

%get the list
serial_port_cell_array = get_serial_ports();

if ismac
    %save battery on macbook
    try
        hostname = char(java.net.InetAddress.getLocalHost.getHostName);
        if ~isempty(hostname) && ischar(hostname) && strcmp(hostname, 'MacBookPro.local')
            fprintf('for MacBook, setting autodisconnect to 10 minutes\n');
            obj.auto_disconnect_minutes = 10;
        end
    catch
    end

    %eliminate non-tty
    for i=length(serial_port_cell_array):-1:1
        if isempty(strfind(serial_port_cell_array{i},'tty'))
            serial_port_cell_array(i) = [];
        end
    end

    %eliminate non-TI
    for i=length(serial_port_cell_array):-1:1
        if isempty(strfind(serial_port_cell_array{i},'TI'))
            serial_port_cell_array(i) = [];
        end
    end

    %if the count remaining is one, assume it is correct and test it
    if length(serial_port_cell_array) == 1
        success = obj.test_possible_serial_connection(serial_port_cell_array{1});
        return; %and exit
    end

elseif isunix
    %isunix is true for mac and linux (mac was already tested for)
    
    %eliminate S0 (locks up matlab on linux)
    for i=length(serial_port_cell_array):-1:1
        if ~isempty(strfind(serial_port_cell_array{i},'S0'))
            serial_port_cell_array(i) = [];
        end
    end
end

if ~isempty(serial_port_cell_array)
    
    %test each serial port found, until succesful
    fprintf('testing available serial port(s):\n');
    for i = 1:length(serial_port_cell_array)
        serial_port = serial_port_cell_array{i};
        if (obj.test_possible_serial_connection(serial_port))
            success = true;
            break;  %break, if successful
        end
    end
else
    fprintf(2, 'no serial ports found to test\n');
end

end