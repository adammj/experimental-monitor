function packet_processor(obj, input_struct, varargin)
            
%see if the correct input was received
if ~isempty(input_struct) && isstruct(input_struct) && isfield(input_struct, 'event_code_queue')
    slots_available = input_struct.event_code_queue.available;
    %fprintf('avail %i\n', slots_available);

    %check if there is information about what was sent
    if isfield(input_struct.event_code_queue, 'sent_id')
        %extract what was sent
        packet_id = input_struct.event_code_queue.sent_id;
        packet_i = input_struct.event_code_queue.sent_i;
        packet_count = input_struct.event_code_queue.sent_count;

        %extract what was supposed to be sent
        peak_queue_object = obj.packet_queue.peak;
        peak_is_valid = true;
        try
            last_id = str2double(peak_queue_object(10:11));
            last_i = str2double(peak_queue_object(13:16));
            last_count = str2double(peak_queue_object(18:21));
        catch
            peak_is_valid = false;
            fprintf(2, 'peak packet was not valid\n');
        end

        if peak_is_valid
            %check if packet matches what was expected
            if (last_id == packet_id) && (last_i == packet_i) && (last_count == packet_count)
                fprintf('packet sent\n');

                %remove the packet from the buffer
                peak_queue_object = obj.packet_queue.read; %#ok<NASGU>

                obj.is_waiting_for_packet_confirmation = false;
            else
                %flag error
                fprintf(2, 'error, unknown packet processed (%i, %i) (%i, %i) (%i, %i)\n', last_id, packet_id, last_i, packet_i, last_count, packet_count);

                fprintf(2, 'sending packet again...\n');
                %send the peaked object (will not remove until successful)
                obj.send_command(obj.commands.send_event_codes, "packet", peak_queue_object);
                obj.is_waiting_for_packet_confirmation = true;

            end
        end
    end


    %check if there are any objects
    used = obj.packet_queue.used;
    if used == 0
        fprintf('packet queue is now empty\n');
    else
        if obj.is_waiting_for_packet_confirmation
            fprintf('waiting for packet confirmation before sending more\n');
            return;
        end

        fprintf('there are %i packets remaining\n', used);

        peak_queue_object = obj.packet_queue.peak;
        %fprintf('available slots: %i, next packet length: %i\n', slots_available, length(peak_queue_object));

        %make sure there is at least 1 extra space
        if length(peak_queue_object) <= slots_available
            fprintf('sending packet...\n');

            %send the peaked object (will not remove until successful)
            obj.send_command(obj.commands.send_event_codes, "packet", peak_queue_object);
            obj.is_waiting_for_packet_confirmation = true;
        else
            fprintf('not enough space for packet, wait...\n');
        end
    end
end

end
