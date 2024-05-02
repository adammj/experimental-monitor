function [value] = safe_GetSecs()
%if GetSecs (PTB) exists, then it uses it
%otherwise, it uses the normal matlab value
%these two values are not the same, although the delta between any two
%will be in seconds

try
    value = GetSecs;
catch
    %convert to seconds
    value = datenummx(clock)*24*3600;
end

end
