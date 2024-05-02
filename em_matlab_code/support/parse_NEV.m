function [output] = parse_NEV(NEV_struct, chunk_split_sec)

%% verify struct is a valid NEV structure
assert(isfield(NEV_struct, 'Data'))
assert(isfield(NEV_struct.Data, 'SerialDigitalIO'))
assert(isfield(NEV_struct.Data.SerialDigitalIO, 'TimeStampSec'))
assert(isfield(NEV_struct.Data.SerialDigitalIO, 'InsertionReason'))
assert(isfield(NEV_struct.Data.SerialDigitalIO, 'UnparsedData'))

if nargin < 2
    chunk_split_sec = 0.2;
end

%% simplify data from NEV
fprintf('simplify\n');
data = NEV_struct.Data.SerialDigitalIO;
data = double([data.TimeStampSec(:), double(data.UnparsedData(:)), double(data.InsertionReason(:))]);

%% create the output (with some defaults)
output = struct();
output.raw_ascii = [];
output.event_codes = [];
output.all_printable_ascii = [];
output.packets = struct([]);
output.structs = struct([]);
output.remaining_printable_ascii = [];
output.chunked_printable_ascii = [];
output.raw_serial_io = [];
output.processed_serial_io = [];

%% separate the data types
fprintf('split\n');
raw_ascii = data;
raw_ascii(raw_ascii(:,3) ~= 1, :) = [];
raw_serial_io = data;
raw_serial_io(raw_serial_io(:,3) == 1, :) = [];


%% store raw
fprintf('store raw\n');
output.raw_ascii = raw_ascii;
output.raw_serial_io = raw_serial_io;

%% store event_codes
fprintf('store event codes\n');
event_codes = raw_ascii;
output.event_codes = struct([]);

event_codes(event_codes(:,2) < 128,:) = [];
if ~isempty(event_codes)

    %create end points
    output.event_codes(size(event_codes, 1),1).time = 0;
    output.event_codes(size(event_codes, 1),1).code = 0;
    output.event_codes(size(event_codes, 1),1).reason = 0;
    output.event_codes(size(event_codes, 1),1).description = char.empty;

    %quickly store to structure array
    temp = num2cell(event_codes(:, 1));
    [output.event_codes(:,1).time] = temp{:};
    
    temp = num2cell(event_codes(:, 2));
    [output.event_codes(:,1).code] = temp{:};
    
    temp = num2cell(event_codes(:, 3));
    [output.event_codes(:,1).reason] = temp{:};
end


%% store printable_ascii
fprintf('store ascii\n');
printable_ascii = raw_ascii;
output.all_printable_ascii = struct([]);

printable_ascii(printable_ascii(:,2) > 127,:) = [];
if ~isempty(printable_ascii)
    
    %create end points
    output.all_printable_ascii(size(printable_ascii, 1),1).time = 0;
    output.all_printable_ascii(size(printable_ascii, 1),1).char = char(0);
    output.all_printable_ascii(size(printable_ascii, 1),1).reason = 0;

    %quickly store to structure array
    temp = num2cell(printable_ascii(:, 1));
    [output.all_printable_ascii(:,1).time] = temp{:};

    temp = num2cell(printable_ascii(:, 2));
    temp = cellfun(@(x) char(x), temp, 'UniformOutput', false); %keep as cell
    [output.all_printable_ascii(:,1).char] = temp{:};

    temp = num2cell(printable_ascii(:, 3));
    [output.all_printable_ascii(:,1).reason] = temp{:};
end

%return

%% look for packets in printable_ascii
fprintf('search for packets\n');
packet_count = 0;
skip_i = 0;
atleast_one_malformed = false;

while ~isempty(strfind(printable_ascii(:,2)', '{"pack"')) %#ok<STREMP>
    
    pack_start_i_list = strfind(printable_ascii(:,2)', '{"pack"');
    if (skip_i + 1) > length(pack_start_i_list)
        fprintf('there is at least one malformed packet (not a serious error)\n');
        atleast_one_malformed = true;
        break; %try this
    else
        %fprintf('%i %i\n', skip_i, length(pack_start_i_list))
        pack_start_i = pack_start_i_list(skip_i + 1);
    end
    
    possible_ends = strfind(printable_ascii(:,2)', '}');
    
    %remove the possible ends before
    possible_ends(possible_ends < pack_start_i) = [];
    
    %remove the possible ends after the enxt pack
    if length(pack_start_i_list) > (skip_i + 1)
        possible_ends(possible_ends > pack_start_i_list(skip_i + 2)) = [];
    end
    
    %fprintf('found pack, end count: %i, skip: %i\n', length(possible_ends), skip_i);
    correct_end = 0;
    for i = 1:length(possible_ends)
        json_string = char(printable_ascii(pack_start_i:possible_ends(i),2)');
        if contains(json_string, "crc")
            try
                json_struct = jsondecode(json_string);
                %fprintf('good: %s\n', json_string);
                correct_end = possible_ends(i);
                break;
            catch
                %fprintf('bad: %s\n', json_string);
            end
        end
    end
    
    
    if correct_end
        %fprintf('match: %i\n', correct_end);
        
        %create the structure first
        if isempty(fieldnames(output.packets))
            output.packets(1,1).time = 0;
            output.packets(1,1).struct_id = [];
            output.packets(1,1).struct_name = '';
            output.packets(1,1).shard_i = [];
            output.packets(1,1).shard_count = [];
            output.packets(1,1).packet = '';
            output.packets(1,1).crc_match = false;
            output.packets(1,1).valid_group = false;
            output.packets(1,1).converted = false;
            output.packets(1,1).fixed_packet = 0;
        end
        
        packet_count = packet_count + 1;
        output.packets(packet_count, 1).time = printable_ascii(pack_start_i, 1);
        output.packets(packet_count, 1).packet = json_string;
        output.packets(packet_count, 1).struct_id = json_struct.pack(1);
        try
            output.packets(packet_count, 1).struct_name = json_struct.name;
        catch
            output.packets(packet_count, 1).struct_name = '';
        end
        output.packets(packet_count, 1).shard_i = json_struct.pack(2);
        output.packets(packet_count, 1).shard_count = json_struct.pack(3);
        
        start_i = strfind(json_string, 'shard') + 8;
        end_i = strfind(json_string, 'crc') - 4;
        if isempty(start_i)
            fprintf('shard not found\n');
        end
        if isempty(end_i)
            fprintf('crc not found\n');
        end

        %test the crc
        shard = json_string(start_i:end_i);
        crc32 = djb2_hash(shard);
        crc_string = crc32_to_string(crc32);
        crc_match = strcmp(json_struct.crc, crc_string);
        
        if ~crc_match
           %try using old hash instead
           crc32 = djb2_hash(shard, true);
           crc_string = crc32_to_string(crc32);
           crc_match = strcmp(json_struct.crc, crc_string);
        end
        
        if ~crc_match
            fprintf('packet does not match crc (id: %i, shard: %i)\n', json_struct.pack(1), json_struct.pack(2));
            new_shard = fix_shard(shard, json_struct.crc);
            
            if ~isempty(new_shard)
               fprintf('fixed packet (id: %i, shard: %i, fix_type: replacement character)\n', json_struct.pack(1), json_struct.pack(2)); 
               crc_match = true;
               json_string(start_i:end_i) = new_shard;
               output.packets(packet_count, 1).packet = json_string;
            end
        end
        output.packets(packet_count, 1).crc_match = crc_match;
        
        %unknown right now
        output.packets(packet_count, 1).valid_group = false;
        output.packets(packet_count, 1).fixed_packet = 0;
        output.packets(packet_count, 1).converted = false;
        
        %remove from printable_ascii
        printable_ascii(pack_start_i:correct_end,:) = [];
        %fprintf('removed\n');
        skip_i = 0;
    else
        skip_i = skip_i + 1;
        %error_count = error_count + 1;
    end
end


%% try to fix malformed packets
if atleast_one_malformed
    fprintf('trying to fix malformed packets\n');
    
    %temporary
    output.remaining_printable_ascii = struct([]);
    for i = 1:size(printable_ascii, 1)
        output.remaining_printable_ascii(i, 1).time = printable_ascii(i, 1);
        output.remaining_printable_ascii(i, 1).char = char(printable_ascii(i, 2));
        output.remaining_printable_ascii(i, 1).reason = printable_ascii(i, 3);
    end
    
    skip_i = 0;
    while ~isempty(strfind(printable_ascii(:,2)', '{"pack"')) %#ok<STREMP>
    
        pack_start_i_list = strfind(printable_ascii(:,2)', '{"pack"');
        if (skip_i + 1) > length(pack_start_i_list)
            fprintf(2, 'error, there is still at least one malformed packet\n');
            break; %try this
        else
            %fprintf('%i %i\n', skip_i, length(pack_start_i_list))
            pack_start_i = pack_start_i_list(skip_i + 1);
        end

        possible_ends = strfind(printable_ascii(:,2)', '}');

        %remove the possible ends before
        possible_ends(possible_ends < pack_start_i) = [];

        %remove the possible ends after the enxt pack
        if length(pack_start_i_list) > (skip_i + 1)
            possible_ends(possible_ends > pack_start_i_list(skip_i + 2)) = [];
        end

        %fprintf('found pack, end count: %i, skip: %i\n', length(possible_ends), skip_i);
        correct_end = 0;
        fix_type = 0;
        for i = 1:length(possible_ends)
            json_string = char(printable_ascii(pack_start_i:possible_ends(i),2)');
            if contains(json_string, "crc")
                try
                    json_struct = jsondecode(json_string);
                    %fprintf('good: %s\n', json_string);
                    correct_end = possible_ends(i);
                    break;
                catch
                    %fprintf('bad: %s\n', json_string);
                    json_struct_org = json_string;
                    
                    %try to fix beginning first (""... -> "\"...)
                    json_string = [json_struct_org(1:32), '\', json_struct_org(33:end)];
                    try
                        json_struct = jsondecode(json_string);
                        %fprintf('success fixing beginning\n');
                        correct_end = possible_ends(i);
                        fix_type = 1;
                        break;
                    catch
                    end
                    
                    %try to fix end next (...\" -> ...\"")
                    both_fix_i = strfind(json_struct_org,'","crc":') - 1;
                    json_string = [json_struct_org(1:both_fix_i), '"', json_struct_org(both_fix_i+1:end)];
                    try
                        json_struct = jsondecode(json_string);
                        %fprintf('success fixing end\n');
                        correct_end = possible_ends(i);
                        fix_type = 2;
                        break;
                    catch
                    end
                    
                    %try to fix both
                    both_fix_i = strfind(json_struct_org,'","crc":') - 1;
                    if both_fix_i > 32
                        json_string = [json_struct_org(1:32),'\',json_struct_org(33:both_fix_i), '"', json_struct_org(both_fix_i+1:end)];
                        try
                            json_struct = jsondecode(json_string);
                            %fprintf('success fixing end\n');
                            correct_end = possible_ends(i);
                            fix_type = 3;
                            break;
                        catch
                        end
                        
                    end

                    fprintf('bad: %s\n', json_string);
                end
            end
        end
        
        if correct_end
            
            packet_count = packet_count + 1;
            
            output.packets(packet_count, 1).fixed_packet = fix_type;
            
            output.packets(packet_count, 1).time = printable_ascii(pack_start_i, 1);
            output.packets(packet_count, 1).packet = json_string;
            output.packets(packet_count, 1).struct_id = json_struct.pack(1);
            try
                output.packets(packet_count, 1).struct_name = json_struct.name;
            catch
                output.packets(packet_count, 1).struct_name = '';
            end
            output.packets(packet_count, 1).shard_i = json_struct.pack(2);
            output.packets(packet_count, 1).shard_count = json_struct.pack(3);
            
            fprintf('fixed packet (id: %i, shard: %i, fix_type: %i)\n', json_struct.pack(1), json_struct.pack(2), fix_type);
            
            %test the shard crc
            start_i = strfind(json_string, 'shard') + 8;
            end_i = strfind(json_string, 'crc') - 4;
            if isempty(start_i)
                fprintf('shard not found\n');
            end
            if isempty(end_i)
                fprintf('crc not found\n');
            end

            %adjust the shard for the fix that was made
            shard = json_string(start_i:end_i);
            
            switch fix_type
                case 1
                    shard = json_string(start_i+1:end_i);
                case 2
                    shard = json_string(start_i:end_i-1);
                case 3
                    shard = json_string(start_i+1:end_i-1);
                otherwise
                    fprintf('unrecognized fix_type (%i)\n', fix_type);
            end
            
            %test the crc
            crc32 = djb2_hash(shard);
            crc_string = crc32_to_string(crc32);
            crc_match = strcmp(json_struct.crc, crc_string);

            if ~crc_match
               %try using old hash instead
               crc32 = djb2_hash(shard, true);
               crc_string = crc32_to_string(crc32);
               crc_match = strcmp(json_struct.crc, crc_string);
            end

            output.packets(packet_count, 1).crc_match = crc_match;
            
            %unknown right now
            output.packets(packet_count, 1).valid_group = false;
            output.packets(packet_count, 1).converted = false;
            
            
            %remove from printable_ascii
            printable_ascii(pack_start_i:correct_end,:) = [];
            
            skip_i = 0;
        else
            skip_i = skip_i + 1;
        end
    end
    
    if skip_i == 0
        fprintf('all malformed packets fixed\n');
    end

    times = [output.packets.time];
    all_unique = (length(times) == length(unique(times)));
    if ~isempty(find(isnan(times), 1)) && ~isempty(find(isempty(times), 1)) && all_unique
        fprintf(2, 'cannot reorder fixed packets\n');
    else
    
        %now, reorder the packets to put them in the correct location
        fprintf('reordering fixed packets\n');
        ordering_list = (1:packet_count)';
        ordering_list(:, 2) = times;
        ordering_list = sortrows(ordering_list, 2);
        new_order = ordering_list(:,1);
        output.packets = output.packets(new_order,1);
    end
end

%% search for duplicate packets (should not be a problem)
if size(output.packets, 1) > 1
    packet_i = 1;
    removed_count = 0;
    while 1    
        
        %check if the next packet compeletely matches the first, if so,
        %remove it
        if (output.packets(packet_i).struct_id == output.packets(packet_i+1).struct_id) && ...
            (output.packets(packet_i).shard_i == output.packets(packet_i+1).shard_i) && ...
            (output.packets(packet_i).shard_count == output.packets(packet_i+1).shard_count)
            
            output.packets(packet_i + 1) = [];
            removed_count = removed_count + 1;
        else
            packet_i = packet_i + 1;
        end
        
        if packet_i >= size(output.packets, 1)
           break;
        end
    end
    
    if removed_count > 0
        fprintf(2, '%i duplicate packets removed\n', removed_count);
    end
    packet_count = size(output.packets, 1);
end


%% turn packets into structures
event_codes_struct_count = 0;
struct_count = 0;
if packet_count > 0
    fprintf('decode packets\n');
    
    %% FIXME: needs to work on the struct_id one by one, instead of using find
    packet_i = 1;
    while packet_i <= packet_count
        current_id = output.packets(packet_i).struct_id;
        current_count = 1;
        
        %manually count continguous packets with same struct_id
        if packet_i<packet_count
           for j = (packet_i + 1):packet_count
               if output.packets(j).struct_id == current_id
                   current_count = current_count + 1;
               else
                   break;
               end
           end
        end

        
        %check that the shard_i and shart_count match expected
        valid_group = true;
        for j = 1:current_count
            if output.packets(packet_i - 1 + j).shard_count ~= current_count
                %fprintf('fail 1\n');
                valid_group = false;     
            end
            
            if output.packets(packet_i - 1 + j).shard_i ~= j
                %fprintf('fail 2\n');
                valid_group = false;
            end
        end
        
        %set all for group
        for j = 1:current_count
            output.packets(packet_i - 1 + j).valid_group = valid_group;
        end
        
        %go ahead and extract the structure
        if valid_group
            
            %create cell array of packets
            packets = cell(current_count, 1);
            fix_type = zeros(current_count, 1);
            for j = 1:current_count
                packets{j, 1} = output.packets(packet_i - 1 + j, 1).packet;
                fix_type(j, 1) = output.packets(packet_i - 1 + j, 1).fixed_packet;
            end
            
            %decode
            [struct_name, structure] = decode_packets_to_structs(packets, fix_type);
            
            %make sure something was decoded
            if ~isempty(struct_name)
                struct_count = struct_count + 1;
                
                output.structs(struct_count, 1).time = output.packets(packet_i, 1).time;
                output.structs(struct_count, 1).name = struct_name;
                output.structs(struct_count, 1).struct = structure;
                
                if strcmp(struct_name, 'event_codes')
                    event_codes_struct_count = event_codes_struct_count + 1;
                    event_codes_struct_i = struct_count;
                    if event_codes_struct_count > 1
                        fprintf('warning: found more than one event_codes struct\n');
                    end
                end
                
                
                for k = packet_i:(packet_i-1+current_count)
                    output.packets(k, 1).converted = true;
                end
            end
        end
        
        %skip to new struct_id
        packet_i = packet_i + current_count;
    end
    
    if ~isempty(find([output.packets.converted]==0, 1))
        fprintf(2,'not all packets were converted\n');
    end
    
end


%% label event codes
if event_codes_struct_count == 1
    fprintf('applying event code labels\n');
    %first, get an array of values
    event_codes_fields = fields(output.structs(event_codes_struct_i).struct);
    values = nan(length(event_codes_fields), 1);
    for i = 1:length(event_codes_fields)
        value = getfield(output.structs(event_codes_struct_i).struct, event_codes_fields{i}); %#ok<GFLD>
        if isfloat(value) || isinteger(value)
            values(i) = double(value);
        end
    end
    
    %check if all event codes are unique
    if length(values) == length(unique(values))
        for i = 1:size(event_codes, 1)
            %match the value index to the code
            value_i = find(values == output.event_codes(i, 1).code);
            %if only one value was found, set it
            if ~isempty(value_i) && length(value_i) == 1
                output.event_codes(i, 1).description = event_codes_fields{value_i};
            end
        end
        
    else
        fprintf('there are duplicate event_codes\n');
    end

elseif event_codes_struct_count > 1
    fprintf('found %d event_codes structs, not sure which to use\n', event_codes_struct_count);
end


%% store remaining_printable_ascii
output.remaining_printable_ascii = struct([]);
for i = 1:size(printable_ascii, 1)
    output.remaining_printable_ascii(i, 1).time = printable_ascii(i, 1);
    output.remaining_printable_ascii(i, 1).char = char(printable_ascii(i, 2));
    output.remaining_printable_ascii(i, 1).reason = printable_ascii(i, 3);
end


%% chunk remaining_printable_ascii 
count = 0;
last_time = -100;
output.chunked_printable_ascii = struct([]);
output_string = '';
for i = 1:size(printable_ascii, 1)
    if abs(printable_ascii(i,1)-last_time) > chunk_split_sec
        if ~isempty(output_string)
            output.chunked_printable_ascii(count, 1).time = last_time;
            output.chunked_printable_ascii(count, 1).string = output_string;
            output_string = '';
        end
        
        count = count + 1;
        output_string = cat(2, output_string, char(printable_ascii(i,2)));
        last_time = printable_ascii(i,1);
    else
        output_string = cat(2, output_string, char(printable_ascii(i,2)));
    end
end

%store the remaining chunk (if exists)
if count > size(output.chunked_printable_ascii, 1)
    output.chunked_printable_ascii(count, 1).time = last_time;
    output.chunked_printable_ascii(count, 1).string = output_string;
end


%% parse serial_io (if any)
if ~isempty(raw_serial_io)
    character_array = char(raw_serial_io(:,2))';
    time_array = raw_serial_io(:,1);

    %find all newlines
    all_cr_locations = find(character_array == 10)';    
    %find all tabs
    all_tab_locations = find(character_array == 9)';

    fprintf('converting serial_io data\n');
    
    tabs_between_cr_counts = histcounts(all_tab_locations, all_cr_locations)';
    
    %all rows should be the same length
    if length(unique(tabs_between_cr_counts)) ~= 1
        fprintf('error: row length varies\n');
        return;
    end
        
    column_count = unique(tabs_between_cr_counts);

    tabs_between_cr_temp = discretize(all_tab_locations, all_cr_locations);
    tabs_between_cr_temp(:,2) = (1:length(tabs_between_cr_temp))';
    tabs_between_cr_temp(isnan(tabs_between_cr_temp(:,1)),:) = [];
    tabs_between_cr_temp(:,1) = [];
    tabs_between_cr = reshape(tabs_between_cr_temp,[column_count, length(tabs_between_cr_temp)/column_count])';

    %build matrix (first, between all of the returns)
    processed_serial_io = zeros(length(all_cr_locations) - 1, column_count + 1);

    for i = 1:length(all_cr_locations) - 1
        tabs_for_line = all_tab_locations(tabs_between_cr(i,:)); % tabs_between_cr{i,1});

        processed_serial_io(i,column_count + 1) = time_array(all_cr_locations(i)+1);
        for j = 1:column_count
            if j == 1
                start_i = all_cr_locations(i) + 1;
            else
                start_i = tabs_for_line(j - 1) + 1;
            end

            end_i = tabs_for_line(j) - 1;

            %fprintf('start: %i  end: %i  %s\n', start_i, end_i, character_array(start_i:end_i));

            number = str2double(character_array(start_i:end_i));
            processed_serial_io(i,j) = number;
        end
    end

    %store to output
    output.processed_serial_io = processed_serial_io;
   
end

fprintf('done\n');

end
