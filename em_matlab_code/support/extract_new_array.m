function [number_array, char_array] = extract_new_array(char_array, columns, suppress_error)
%extracts a new string array into a 16-bit number array
%new array only contains brackets around the array, no commas or semicolons

if nargin < 2
    columns = 1;
    if nargin < 3
        suppress_error = false;
    end
end

num_chars = 3;

start_i = strfind(char_array,'[');
end_i = strfind(char_array,']');

%if either the start or end brackets are missing, return
if isempty(start_i) || isempty(end_i)
    if ~suppress_error
        fprintf('missing start or end bracket\n');
    end
    %for failure, array is empty
    number_array = uint16.empty;
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
    %for failure, array is empty
    number_array = uint16.empty;
    return;
end

%get the actual valid array section
valid_array_section = char_array(start_i:end_i);

%trim the char_array
if (end_i+1) < length(char_array)
    char_array = char_array((end_i+2):end);
else
    char_array = '';
end

%find the total number of values
values = length(valid_array_section)/num_chars;
if mod(values,1) ~= 0
    if ~suppress_error
        fprintf('length is invalid\n');
    end
    %for failure, array is empty
    number_array = uint16.empty;
    return;
end

%find the number of rows
rows = values/columns;
if mod(rows,1) ~= 0
    if ~suppress_error
        fprintf('rows are uneven\n');
    end
    %for failure, array is empty
    number_array = uint16.empty;
    return;
end

%past this point, it is assumed it will work

%get the products
bit_mask = uint16([2048, 64, 1]);

%convert to values
valid_array_section = uint16(valid_array_section);
%shift to zero
valid_array_section = valid_array_section - 48;
%shift higher ones to remove gap
valid_array_section(valid_array_section>42) = valid_array_section(valid_array_section>42) - 6;

%reshape and multiple the bit mask
valid_array_section = reshape(valid_array_section, num_chars, [])';
valid_array_section = bsxfun(@times, valid_array_section, bit_mask); %necessary for older versions valid_array_section.*bit_mask;

%sum along the rows
valid_array_section = sum(valid_array_section, 2);

%reshape into x,y array
number_array = uint16(reshape(valid_array_section, columns, []))';

end

