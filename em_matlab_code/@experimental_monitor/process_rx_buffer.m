function process_rx_buffer(obj)
            
%if there is something in the raw_rx_buffer, process it
while obj.raw_rx_buffer.used > 0
    
    obj.rx_char_buffer = cat(2, obj.rx_char_buffer, obj.raw_rx_buffer.read());

    %check the crlf list
    obj.rx_crlf_list = strfind(obj.rx_char_buffer, obj.crlf_char);

    %if there is a crlf, then process the chunk
    while ~isempty(obj.rx_crlf_list)

        %grab the first crlf
        crlf_pos = obj.rx_crlf_list(1);

        %get the data (except crlf, and remove it and crlf from buffer
        buffer_line = obj.rx_char_buffer(1:(crlf_pos-1));
        obj.rx_char_buffer(1:(crlf_pos+1)) = [];

        %only check if there is something there
        if ~isempty(buffer_line)
            %try to encode JSON into a struct, otherwise store it under the field 'text'
            try                
                received_struct = jsondecode(buffer_line);  %Matlab 2016b+
            catch
                received_struct = struct('text', buffer_line);
            end

            obj.received_struct_buffer.write(received_struct);

        end

        %if there isn't a partial buffer, then exit
        if isempty(obj.rx_char_buffer)
            break;
        end

        %otherwise, check for more data
        obj.rx_crlf_list = strfind(obj.rx_char_buffer, obj.crlf_char);
    end
end

%if there is something in the received_struct_buffer, process it
while obj.received_struct_buffer.used > 0

    received_struct = obj.received_struct_buffer.read;

    %get the first fieldname (normally only 1)
    field_name_list = fieldnames(received_struct);
    first_field_name = field_name_list{1};

    if strcmp(first_field_name, 'error')
        %add to error ring buffer
        obj.error_buffer.write(received_struct);

        %print message
        fprintf(2, 'error: %s\n', received_struct.error);

        if size(field_name_list, 1) > 1
            field_name_list = field_name_list(2:end); 
            for i = 1:size(field_name_list, 1)
                field_name = field_name_list{i};
                field_value = received_struct.(field_name);

                if isa(field_value, 'string') || isa(field_value, 'char')
                    fprintf(2, 'error_addl: %s: %s\n', field_name, field_value);
                else

                    if field_value == round(field_value)
                        fprintf(2, 'error_addl: %s: %d\n', field_name, field_value);
                    else
                        fprintf(2, 'error_addl: %s: %f\n', field_name, field_value);
                    end
                end
            end
        end

    else

        %check if there is a history field, and convert it
        if isfield(received_struct.(first_field_name), 'history')
            received_struct.(first_field_name).history = extract_new32_array(received_struct.(first_field_name).history, true);
        end

        %copy the buffer to the receive ringbuffer
        obj.receive_buffer.write(received_struct);

        if strcmp(first_field_name, 'status')
            fprintf('status: %s\n', received_struct.status);

            if size(field_name_list, 1) > 1
                field_name_list = field_name_list(2:end); 
                for i = 1:size(field_name_list, 1)
                    field_name = field_name_list{i};
                    field_value = received_struct.(field_name);

                    if isa(field_value, 'string') || isa(field_value, 'char')
                        fprintf(1, 'status_addl: %s: %s\n', field_name, field_value);
                    else

                        if field_value == round(field_value)
                            fprintf(1, 'status_addl: %s: %d\n', field_name, field_value);
                        else
                            fprintf(1, 'status_addl: %s: %f\n', field_name, field_value);
                        end
                    end
                end
            end
        else

            %test each of the callbacks
            for i = 1:size(obj.callbacks, 1)
                %no good way, right now, to break this early - without
                %adding a flag or queue for callbacks not yet processed
                
                if strcmp(first_field_name, obj.callbacks(i).callback_string)

                    %safely send to callback and measure time
                    %was using delay_call, but overhead is too great
                    try
                        start_tic = tic;
                        feval(obj.callbacks(i,1).function, received_struct, obj.callbacks(i,1).user_struct);
                        elapsed = toc(start_tic);

                        %warn if it takes too long
                        %unless it is the packet processor, which sends long packets
                        if (elapsed > obj.warn_long_callback_sec) && ~strcmp(obj.callbacks(i,1).function_str, 'obj.packet_processor')
                            fprintf(2, 'warning: callback %s took %.3f sec\n', obj.callbacks(i,1).function_str, elapsed);
                        end
                    catch e
                        fprintf(2, 'callback error: %s\n', obj.callbacks(i,1).function_str);
                        fprintf(2, 'callback error: the identifier was: %s\n', e.identifier);
                        fprintf(2, 'callback error: the message was: %s\n', e.message);
                    end
                end
            end
        end
    end
end

end
