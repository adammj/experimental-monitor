function [number_array, char_array] = extract_ascii_array(char_array, columns, suppress_error)
%extracts an ascii string array into a 16-bit number array (assumes 2 bytes
%per number)
%hex array only contains brackets around the array, no commas or semicolons


if nargin < 4
    suppress_error = false;
end

%assume failure
number_array = uint16.empty;

start_i = strfind(char_array,'[');
end_i = strfind(char_array,']');

%if either the start or end brackets are missing, return
if isempty(start_i) || isempty(end_i)
    if ~suppress_error
        fprintf('missing start or end bracket\n');
    end
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
values = length(valid_array_section)/2;
if mod(values,1) ~= 0
    if ~suppress_error
        fprintf('length is invalid\n');
    end
    return;
end

%find the number of rows
rows = values/columns;
if mod(rows,1) ~= 0
    if ~suppress_error
        fprintf('rows are uneven\n');
    end
    return;
end

%past this point, it is assumed it will work

number_array = uint16(valid_array_section);
number_array = reshape(number_array,[],2); %first reshape to do bit math
number_array = bitshift(number_array(:,1),8) + number_array(:,2);
number_array = reshape(number_array,[],columns); %then reshape to final form


end