function [number_array] = extract_new32_array(char_array, suppress_error)
%extracts a string array into a double array (of 16-bit integers)
%new array only contains brackets around the array, no commas or semicolons

if nargin < 2
    suppress_error = false;
end


num_chars = 5;

start_i = strfind(char_array,'[');
end_i = strfind(char_array,']');

%if either the start or end brackets are missing, return
if isempty(start_i) || isempty(end_i)
    if ~suppress_error
        fprintf('missing start or end bracket\n');
    end
    %for failure, array is just the input
    number_array = char_array;
    return;
end

%just grab first, if many
start_i = start_i(1) + 1;
end_i = end_i(1) - 1;

%if there is nothing there, exit
if (end_i - start_i < 0)
    if ~suppress_error
        fprintf('nothing between the brackets\n');
    end
    %for failure, array is just the input
    number_array = char_array;
    return;
end

%get the actual valid array section
valid_array_section = char_array(start_i:end_i);

%find the total number of values
values = length(valid_array_section)/num_chars;
if mod(values,1) ~= 0
    if ~suppress_error
        fprintf('length is invalid\n');
    end
    %for failure, array is empty
    number_array = char_array;
    return;
end

%makesure there are an even multiple of values
if mod(values,1) ~= 0
    if ~suppress_error
        fprintf('not an even multiple of 5\n');
    end
    %for failure, array is empty
    number_array = char_array;
    return;
end

%past this point, it is assumed it will work

%get the products
bit_mask = uint32([33554432, 262144, 2048, 16, 1]);

%convert to values
valid_array_section = uint32(valid_array_section);

%subtract offset
valid_array_section = valid_array_section - 128;

%reshape and multiple the bit mask
valid_array_section = reshape(valid_array_section, num_chars, [])';
%valid_array_section = bsxfun(@times, valid_array_section, bit_mask); %necessary for older versions valid_array_section.*bit_mask;
valid_array_section = valid_array_section.*bit_mask;

%sum along the rows
valid_array_section = sum(valid_array_section, 2);

%split 32bit into 2x16bit
valid_array_section(:,2) = bitand(valid_array_section, 2^16-1);
valid_array_section(:,1) = bitshift(valid_array_section(:,1), -16);

%reshape into vector in correct order (return as double)
number_array = double(reshape(valid_array_section', 1, []))';

end
