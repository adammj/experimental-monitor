function hash = djb2_hash(str, old_ver, bits)
% djb2 hash, which produces the same values as the C++ version
% old_ver is not technically correct, but kept around

%default to using new version
if nargin < 2 || isempty(old_ver)
    old_ver = false;
end

if nargin < 3
    bits = 32;
end

if ~(isa(str, 'char') || isa(str, 'string'))
    fprintf('need string or char array\n');
    return;
end

if isa(str, 'string')
    str = char(str);
end

if ~old_ver

    str = double(str);

    max_value = 2^bits;
    hash = double(5381);

    for i = 1:length(str)
        inner = double(mod(hash * 33, max_value));
        hash = double(mod(bitxor(inner, str(i)), max_value));
    end
    
else
    %old version, which isn't correct, but kept around for compatability
    hash = 5381*ones(size(str,1),1); 

    for i = 1:size(str,2)
        hash = mod(hash * 33 + str(:,i), 2^32-1); 
    end
end

end
