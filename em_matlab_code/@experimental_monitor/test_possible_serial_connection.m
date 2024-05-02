function [success] = test_possible_serial_connection(obj, serial_port)

%assume failure
success = false;

%create a temp serial connection
temp_connection = serial(serial_port);
temp_connection.BaudRate = obj.baud_rate;
temp_connection.DataBits = 8;
temp_connection.StopBits = 1;
temp_connection.UserData = 'experimental_monitor';
temp_connection.Terminator = {'CR/LF','LF'};    %does nothing for read
temp_connection.ErrorFcn = @had_serial_error;
temp_connection.BytesAvailableFcnCount = 1;
temp_connection.BytesAvailableFcnMode = 'terminator';
%NOTE: although it seems like using BytesAvailableFcn should have less
%overhead, the performance is signficantly worse when streaming data than
%using a timer


%setup receive callback and buffer
temp_connection.InputBufferSize = ceil(temp_connection.BaudRate);
temp_connection.OutputBufferSize = ceil(temp_connection.BaudRate);

%open the connection
fprintf('trying to open port ''%s''... ', serial_port);
try 
    fopen(temp_connection); %might fail setting baudrate
catch
    fprintf('failed\n');
    delete(temp_connection);    %clean up the list of serial connections
    return;
end

%wait for device to be open (wait up to 5 seconds)
start_tic = tic;
status = temp_connection.Status;
while ~strcmp(status, 'open') && toc(start_tic) < 5
    pause(0.05); drawnow;  %flush java events
    status = temp_connection.Status;
end

if ~strcmp(status, 'open')
    fprintf('failed, waiting for port to finish opening\n');
    return;
end

%show that port was opened
fprintf('open\n');

%clear the serial buffer
serial_receive(temp_connection);

%testing connection (make sure the device responds as expected)
fprintf('testing connection (up to 5 sec)... ');

%wait for bytes to be received (wait upto 5 seconds)
start_tic = tic;

%send the the test connection command
obj.serial_transmit('{"command":"test_connection"}', temp_connection);

%append to a buffer, in case messages are broken up
buffer = serial_receive(temp_connection);
while toc(start_tic) < 5 && isempty(strfind(buffer,'{"connection":"success"')) %#ok<STREMP>
    pause(0.01);
    buffer = [buffer, serial_receive(temp_connection)]; %#ok<AGROW>
end

%test if proper response was received
if ~isempty(strfind(buffer,'{"connection":"success"')) %#ok<STREMP>
    %if successful
    fprintf('success\n');
    
    %copy the serial_connection, and enable everything
    obj.serial_connection = temp_connection;    
    success = true; %only successful outcome
else
    fprintf(2, '\nmatlab error: device did not respond as expected (try resetting device)\n');
    fclose(temp_connection);
    delete(temp_connection);
end

end


function [buffer, buffer_len] = serial_receive(alternative_connection)

java_serial_object = igetfield(alternative_connection, 'jobject');

%java methods for speed
buffer_len = get(java_serial_object, 'BytesAvailable');
%fprintf('found: %i\n', buffer_len);

if buffer_len
    buffer = fread(java_serial_object, buffer_len, 0, 0);
    buffer = char(buffer(1)');
else
    buffer = '';
end

end

function had_serial_error(varargin)
fprintf(2, 'matlab error: had serial port error\n');     

if nargin > 1
    for i = 1:length(varargin)
       varargin(1)
    end
end
    
end
