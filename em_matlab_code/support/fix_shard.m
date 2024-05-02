function [corrected_shard] = fix_shard(shard, crc_desired, bits)

if nargin < 3
    bits = 32;
end

corrected_shard = [];

%if provided in the string form, then convert
if ischar(crc_desired) || isstring(crc_desired)
    crc_desired = char(crc_desired);
    crc_desired = crc32_to_string(crc_desired, true);
end

crc_error = djb2_hash(shard, [], bits);

if crc_error == crc_desired
    fprintf('shard is already correct\n');
    corrected_shard = shard;
    return;
else
    fprintf('try to fix shard\n');
    
    %first, create array of hashes
    forward_hash_array = zeros(length(shard), 1);
    distance_array = (1:length(shard))';
    
    shard_double = double(shard);
    max_value = 2^bits;
    hash = double(5381);

    for i = 1:length(shard_double)
        inner = double(mod(hash * 33, max_value));
        hash = double(mod(bitxor(inner, shard_double(i)), max_value));
        forward_hash_array(i) = hash;
    end
    
    current_hash = crc_desired;
    
    %go through each character, backwards
    for i = length(shard):-1:1

        distance_array(i, 2) = bitxor(current_hash, forward_hash_array(i));
        
        xor_value = mod(bitxor(current_hash, double(uint8(shard(i)))), 2^bits);

        multiplier = 0;
        if mod(xor_value/33,1) ~= 0
            multipliers = ((xor_value + 2^bits*(1:64)')/33 == round((xor_value + 2^bits*(1:64)')/33)).*(1:64)';
            multipliers(multipliers == 0) = [];
            if ~isempty(multipliers)
                multiplier = multipliers(1);
            else
                fprintf(2, 'no multiplier\n');
                break;
            end
        end

        current_hash = mod((xor_value+2^bits*multiplier)/33, 2^bits);
    end
    
    %filter distances
    try
        %sort in ascending size
        distance_array = sortrows(distance_array, 2);
        distance_array(distance_array(:,2) > 127, :) = [];  %remove all greater than 127 away
    catch
        distance_array = [];
    end
    
    %if there is at least one to check, do so
    if ~isempty(distance_array)
        
        for j = 1:size(distance_array, 1)
            probable_location = distance_array(j, 1);
            fprintf('probable character error at location %i: (%s)\n', probable_location, shard(probable_location));

            %test all valid ascii char
            for i = 1:127
                possible_shard = shard;
                possible_shard(probable_location) = char(i);
                crc_fix = djb2_hash(possible_shard, [], bits);

                if crc_fix == crc_desired
                    fprintf('found replacement character (%s), shard fixed\n', char(i));
                    corrected_shard = possible_shard;
                    return;
                end
            end
            fprintf('no solutions with that character\n');
        end
    end
    
    fprintf(2, 'shard not fixed\n');
end

end