function add_packets_to_queue(obj, packets)
            
assert(iscell(packets))

%check if there are packets
if ~isempty(packets)

    %take the cell array and add to the buffer
    if length(packets) > obj.packet_queue.available
        fprintf('structure is too large for queue\n');
        return;
    end

    if obj.packet_queue.used == 0
        %make sure the callback is enabled
        obj.add_callback('event_code_queue', @obj.packet_processor, [], true);
        obj.is_waiting_for_packet_confirmation = false;
    end
    
    packet_count = length(packets);
    fprintf('add %i event code packets to queue\n', packet_count);
    
    for i = 1:packet_count
        current_packet = packets{i};
        assert(isstring(current_packet) || ischar(current_packet))
        obj.packet_queue.write(current_packet);
    end

else
   fprintf('no packets, possible error\n'); 
end

end
