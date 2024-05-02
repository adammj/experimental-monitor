function [output] = crc32_to_string(input, inverse_yes)

if nargin < 2
    inverse_yes = false;
end

if ~inverse_yes
    assert(input >= 0 && input <= (2^32-1));
    assert(mod(input, 1) == 0);
    
    output = '000000';
    bin_string = dec2bin(input, 32);
    des_len = 6;

    for i = 1:6
        if i < 6
            bits = bin_string(((i-1)*des_len + 1):(i*des_len));
        else
            bits = [bin_string(((i-1)*des_len + 1):end), '0000'];   %fill extra bits
        end

        %start at 0 and skip over the backslash (not allowed in JSON)
        dec = bin2dec(bits) + 48;
        dec(dec > 91) = dec(dec > 91) + 1;

        %convert to char
        output(i) = char(dec);
    end

else
    assert(ischar(input) || isstring(input));
    input = char(input);
    assert(length(input) == 6);
    
    output = uint8(input);
    output(output > 91) = output(output > 91) - 1; 
    output = output - 48;
    output = dec2bin(output, 6);
    output = reshape(output', [], 1)';
    output = bin2dec(output(1:32));
    
end


end
