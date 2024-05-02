function [packets] = encode_struct_to_packets(input_struct, struct_name)

%verify input
assert(isstruct(input_struct), 'input_struct must be a structure');
assert((ischar(struct_name) || isstring(struct_name)) && ~isempty(struct_name), 'struct_name must be a char array or string');
assert(~isempty(struct_name) && length(struct_name) <= 20, 'struct_name must be 1 to 20 char');


%% encode struct into escaped string (prepare for JSON)
encoded_string = jsonencode(input_struct);
encoded_struct = struct();
encoded_struct.struct = encoded_string;
encoded_string = jsonencode(encoded_struct);
escaped_string = encoded_string(12:(end-2));

%% encode into packets
persistent id
if isempty(id)
    id = 1;
else
    id = id + 1;
    %only two digits (but also max 6 bits)
    if id == 64
        id = 1;
    end
end

packets = {};
%defined ahead of time, with fixed lenth parameters
%packet id+range is 14 for max 9999 packets: [ID,0001,9999]
%name is 1 to 20 char
%shard is largest possible so that each packet is 1024
%hash is 6 char

char_for_name_param = 10;   
max_extra_packet_len = 975;
max_first_packet_len = max_extra_packet_len - char_for_name_param - length(struct_name);

packet_indexes = [];


%first, break up shards
if length(escaped_string) <= max_first_packet_len
    packet_indexes(1, 1) = 1;
    packet_indexes(1, 2) = length(escaped_string);
else
    string_i = 0;
    while string_i < length(escaped_string)
        %if first packet
        if isempty(packet_indexes)
            
            %move to max possible length
            string_i = string_i + max_first_packet_len;
            
            %if the last character is the escape character, go back
            while escaped_string(string_i) == '\'
                string_i = string_i - 1;
                
            end
            
            %store indexes
            packet_indexes(1, 1) = 1;
            packet_indexes(1, 2) = string_i;
            
        else
            %move to max possible length
            string_i = string_i + max_extra_packet_len;
            
            %if the location is past the end, then move to end
            if string_i >= length(escaped_string)
                string_i = length(escaped_string);
            else
                %if the last character is the escape character, go back
                while escaped_string(string_i) == '\'
                    string_i = string_i - 1;
                end
            end
            
            %store indexes
            packet_indexes(end + 1, 1) = packet_indexes(end, 2) + 1; %#ok<AGROW>
            packet_indexes(end, 2) = string_i;
        end
    end
end

%store the count
packet_count = size(packet_indexes, 1);

if packet_count > 9999
    fprintf('Actual packet count (%i) is greater than the max allowed of 9999\n', packet_count);
    return;
end

%create empty cell array
packets = cell(packet_count, 1);

for packet_i = 1:packet_count
    shard = escaped_string(packet_indexes(packet_i, 1):packet_indexes(packet_i, 2));
    crc_string = crc32_to_string(djb2_hash(shard));
    padding = 0;
    
    %if the first packet
    if packet_i == 1
        %check padding (only if more than one packet)
        if packet_count > 1
            padding = max_first_packet_len - length(shard);
        end
        padding = char(32*ones(1, padding));
        
        packets{packet_i, 1} = ['{"pack":[', num2str(id, '%02d'), ',', num2str(packet_i, '%04d'), ',', num2str(packet_count, '%04d'), '],"name":"', struct_name, '","shard":"', shard, '"', padding ,',"crc":"', crc_string, '"}'];
    else
        %check padding (only if not the last packet)
        if packet_i < packet_count
            padding = max_extra_packet_len - length(shard);
        end
        padding = char(32*ones(1, padding));
        
        packets{packet_i, 1} = ['{"pack":[', num2str(id, '%02d'), ',', num2str(packet_i, '%04d'), ',', num2str(packet_count, '%04d'), '],"shard":"', shard, '"', padding ,',"crc":"', crc_string, '"}'];
    end
    
end

end
