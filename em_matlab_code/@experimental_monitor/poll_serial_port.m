function poll_serial_port(obj, varargin)
%NOTE: although it seems like using BytesAvailableFcn should have less
%overhead, the performance is signficantly worse when streaming data than
%using a timer

bytes_available = get(obj.java_serial_object, 'BytesAvailable');

if bytes_available > 0
    
    char_buffer = fread(obj.java_serial_object, bytes_available, 0, 0);
    
    %just returned values (horizontal)
    values = char_buffer(1)';
    
    %must first recast to uint8 (to recover negative values), then cast to char
    values_uint8 = typecast(values, 'uint8');
    values_char = char(values_uint8);
    
    %add to the buffer and record the time
    obj.raw_rx_buffer.write(values_char);
    obj.last_receive_timestamp = datenummx(clock);

    %was using delay_call, but total overhead was too great
    %instead, safely call process_rx_buffer
    try
        obj.process_rx_buffer;
    catch
    end
end

end
