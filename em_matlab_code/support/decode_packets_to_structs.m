function [struct_name, structure, unused_packets, error_packets] = decode_packets_to_structs(packets, fix_type)

%verify input is a cell
assert(iscell(packets))

if nargin < 2
    fix_type = zeros(size(packets));
end

assert((size(packets,1) == size(fix_type,1)) && (size(packets,2) == size(fix_type,2)))

%create the empty output
struct_name = '';
structure = struct();
unused_packets = {};
error_packets = {};

%check the inputs closer
if isempty(packets)
    return;
end

if ~isvector(packets)
    fprintf('packets are not in a vector\n');
    return;
end

%make sure it is in a column
if ~iscolumn(packets)
    packets = packets';
end


%set up defaults for working on the queue
current_id = [];
current_packets = {};
running_string = '';
escaped_string = '';
count = 0;
%total_count = length(packets);
packets_org = packets;

while ~isempty(packets)
    count = count + 1;
    %fprintf('%i / %i\n', count, total_count);
    
    %copy the top packet to the current
    current_packet = packets{1};
    
    try
        packet_struct = jsondecode(current_packet);
    catch
        fprintf('error decoding packet %i\n', count);
        error_packets = cat(1, error_packets, current_packet);
        packets(1) = [];
        continue;
    end
    
    %add to current list and remove from queue
    current_packets = cat(1, current_packets, current_packet);
    packets(1) = [];
    
    %copy the id
    id = packet_struct.pack(1);

    %if the current id is blank, then set
    if isempty(current_id)
        if packet_struct.pack(2) == 1
            current_id = id;
            current_shard = packet_struct.pack(2);
            struct_name = packet_struct.name;
        else
            fprintf('first packet was not 1 of X\n');
            error_packets = cat(1, error_packets, current_packet);
            packets(1) = [];
            continue;
        end            
    else
        if id ~= current_id
            fprintf('error, id does not match current_id\n');
            return;
        end

        if packet_struct.pack(2) ~= (current_shard + 1)
            fprintf('error, shards out of order\n');
            return;
        else
            current_shard = packet_struct.pack(2);
        end
    end
    
    start_i = strfind(current_packet, 'shard') + 8;
    %find the double quotes before the crc (accounting for padding)
    end_i = strfind(current_packet, '"crc');
    quote_i = find(current_packet == '"');
    quote_i(quote_i >= end_i) = [];
    end_i = quote_i(end) - 1;
    
    if isempty(start_i)
        fprintf('shard not found\n');
    end
    if isempty(end_i)
        fprintf('crc not found\n');
    end
    
    shard = current_packet(start_i:end_i);
    
    %adjust shard if packet was fixed
    switch fix_type(count)
        case 0
            %no change
        case 1
            shard = current_packet(start_i+1:end_i);
        case 2
            shard = current_packet(start_i:end_i-1);
        case 3
            shard = current_packet(start_i+1:end_i-1);
        otherwise
            fprintf('unrecognized fix_type (%i)\n', fix_type(count));
    end
    
    %check crc of shard
    crc32 = djb2_hash(shard);
    crc_string = crc32_to_string(crc32);
    crc_match = strcmp(packet_struct.crc, crc_string);
    
    if ~crc_match
        %try using old hash instead
        crc32 = djb2_hash(shard, true);
        crc_string = crc32_to_string(crc32);
        crc_match = strcmp(packet_struct.crc, crc_string);
        
        %if also a bad match, then restore the original crc
        if ~crc_match
            crc32 = djb2_hash(shard);
            crc_string = crc32_to_string(crc32);
        end
    end
    
    
    if crc_match
        running_string = cat(2, running_string, shard);
    else
        fprintf('error with crc (stored:"%s" calculated:"%s") for packet (id: %i, shard: %i)\n', packet_struct.crc, crc_string, packet_struct.pack(1), packet_struct.pack(3));
    end
    
    %check if there was a full back, set and go
    if (packet_struct.pack(2) == packet_struct.pack(3)) 
        %fprintf('full packet, set and go\n');
       
        %store outputs
        escaped_string = running_string;
        unused_packets = packets;
        break;
    end
end

%remove consumed packets
%decoder_struct.packet_queue(1:last_packet_to_delete) = [];

structure = decode_struct(escaped_string);
%if the structure is empty, clear the struct_name
try
    if isempty(fields(structure))
        struct_name = '';
        error_packets = packets_org;
    end
catch
    struct_name = '';
    error_packets = packets_org;
end


end

function [decoded_struct] = decode_struct(escaped_string)

try
    %decode, using the escaped string
    decoded_string = ['{"struct":"', escaped_string, '"}'];
    decoded_struct = jsondecode(decoded_string);
    decoded_struct = jsondecode(decoded_struct.struct);
catch
    fprintf('failure decoding\n');
    decoded_struct = struct();
end

end