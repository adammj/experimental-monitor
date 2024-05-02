function [port_list] = get_serial_ports()
%return a cell array list of available serial ports
%will try to use the 2017a function, seriallist

%try to use the new seriallist function (2017a), otherwise just use the old method
try
    string_port_list = seriallist;  %returns a list of strings
    port_list = cell(0,1);
    for i = 1:length(string_port_list)
        %convert string to char
        port_list{i,1} = char(string_port_list(i));
    end
catch
    port_list = get_serial_ports_old();
end

if ispc
    %pc typically has the em at the end of the COM ports
    port_list = flip(port_list);
end

end

%old method
function [port_list] = get_serial_ports_old()
try
    bad_serial = serial('IMPOSSIBLE_NAME_ON_PORT');
    fopen(bad_serial);
catch
    error_msg = lasterr; %#ok<LERR>
end

%Start of the COM available port
if ispc
    start_index = strfind(error_msg,'COM');   %pc uses COM
else
    start_index = strfind(error_msg,'/dev');  %unix/mac uses /dev
end

%End of COM available port
end_index = strfind(error_msg,'Use')-3;

port_list_str = error_msg(start_index:end_index);

%Parse the resulting string
port_list_commas = strfind(port_list_str,',');

% If no Port are available
if isempty(start_index)
    port_list{1}='';
    return;
end

% If only one Port is available
if isempty(port_list_commas)
    port_list{1}=port_list_str;
    return;
end

port_list{1} = port_list_str(1:port_list_commas(1)-1);

for i=1:numel(port_list_commas)+1
    % First One
    if (i==1)
        port_list{1,1} = port_list_str(1:port_list_commas(i)-1);
    % Last One
    elseif (i==numel(port_list_commas)+1)
        port_list{i,1} = port_list_str(port_list_commas(i-1)+2:end);        %#ok<AGROW>
    % Others
    else
        port_list{i,1} = port_list_str(port_list_commas(i-1)+2:port_list_commas(i)-1); %#ok<AGROW>
    end
end

end
