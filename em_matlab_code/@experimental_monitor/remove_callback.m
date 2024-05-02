function remove_callback(obj, callback_id_or_all_or_string, silent)

if nargin < 3
    silent = false;
end

if isstring(callback_id_or_all_or_string)
    callback_id_or_all_or_string = char(callback_id_or_all_or_string);
end

%if a logical and true, or if a char array and "all", then remove all
if (islogical(callback_id_or_all_or_string) && callback_id_or_all_or_string) || (ischar(callback_id_or_all_or_string) && strcmp(callback_id_or_all_or_string, "all"))
    %if nargin has all, then remove all
    obj.callbacks = struct([]);
    if ~silent
        fprintf('removed all callbacks\n');
    end
    return;
end

%if it is a string, return a list of matching callback_ids
%if ischar(callback_id_or_all_or_string)



%try to find the callback_id
found_one = false;
for i = size(obj.callbacks, 1):-1:1
    if ischar(callback_id_or_all_or_string)
        if strcmp(obj.callbacks(i).callback_string, callback_id_or_all_or_string)
            found_one = true;
            remove_callback_i(obj, i, silent);
        end
    else
        if obj.callbacks(i).callback_id == callback_id_or_all_or_string
            found_one = true;
            remove_callback_i(obj, i, silent);
        end
    end
end

if ~found_one && ~silent
    if ischar(callback_id_or_all_or_string)
        fprintf('callback_string ''%s'' is not assigned to a callback function\n', callback_id_or_all_or_string);    
    else
        fprintf('callback_id %d is not assigned to a callback function\n', callback_id_or_all_or_string);    
    end
end

end

function remove_callback_i(obj, callback_i, silent)
    %copy
    temp_callbacks = obj.callbacks;
    callback_string = obj.callbacks(callback_i, 1).callback_string;
    callback_id = obj.callbacks(callback_i, 1).callback_id;
    
    %move each up one
    for i = callback_i:(size(temp_callbacks, 1) - 1)
        temp_callbacks(i, 1) = temp_callbacks(i + 1, 1);
    end

    %delete last
    temp_callbacks(end, :) = [];

    %store
    obj.callbacks = temp_callbacks;
    
    if ~silent
        fprintf('removed callback_id: %d for callback_string: ''%s''\n', callback_id, callback_string);
    end

end
