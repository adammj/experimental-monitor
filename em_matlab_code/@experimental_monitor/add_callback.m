function [new_callback_id] = add_callback(obj, callback_string, function_h, user_struct, silent)
assert(isa(function_h, 'function_handle'), 'a valid function handle must be provided')

if nargin < 5
    silent = false;
    if nargin < 4
        user_struct = struct();
    end
end

%catch if an empty user_struct was provided
if isempty(user_struct)
    user_struct = struct();
end
assert(isstruct(user_struct), 'user_struct must be a structure')

%test recieve_string
if nargin < 3
    fprintf('must provide a recieve_string for a callback\n')
    return;
end

assert(isa(callback_string, 'string') || isa(callback_string, 'char'), 'callback_string must be a string or char array')
assert(~isempty(callback_string), 'callback_string cannot be empty')
callback_string = char(callback_string);

%see if same function and callback_string pair are already in use
i = 1;
while i <= size(obj.callbacks, 1)
    
    %if callback string and function matches
    if strcmp(obj.callbacks(i, 1).callback_string, callback_string) && strcmp(func2str(obj.callbacks(i,1).function), func2str(function_h))
        if ~silent
            fprintf('there is already a callback for this same callback string and function, callback_id: %d. removing old one.\n', obj.callbacks(i,1).callback_id);
        end

        %remove the old callback first
        old_id = obj.callbacks(i,1).callback_id;
        obj.remove_callback(old_id, true);  %silently
    else
        i = i + 1;
    end
end

%find an available id (randomly selected)
id_is_used = true;
while id_is_used
    new_callback_id = randi([1000,9999]);
    id_is_used = false;
    for i = 1:size(obj.callbacks, 1)
        if obj.callbacks(i, 1).callback_id == new_callback_id
            id_is_used = true;
        end
    end
end

next_i = size(obj.callbacks, 1) + 1;

%copy the callbacks
temp_callbacks = obj.callbacks;

%get simplified function_str
function_str = func2str(function_h);
function_str = strrep(function_str, '@(varargin)', '');
function_str = strrep(function_str, '(varargin{:})', '');

%add the new callback
temp_callbacks(next_i, 1).callback_id = new_callback_id;
temp_callbacks(next_i, 1).callback_string = callback_string;
temp_callbacks(next_i, 1).function_str = function_str;
temp_callbacks(next_i, 1).function = function_h;
temp_callbacks(next_i, 1).user_struct = user_struct;

%store
obj.callbacks = temp_callbacks;

if ~silent
    fprintf('added callback for function: %s for callback_string: %s\n', func2str(function_h), callback_string);
end

end
