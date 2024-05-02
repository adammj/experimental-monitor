function [peaks] = get_peaks(input, min_peak_height, min_separation)
% find all peaks in a given input
% returns: 3 column vector with the location, amplitude, width
% warnings are disabled to prevent erroneous warnings

if nargin < 3
    min_separation = 1;
end

assert(isvector(input))
assert(length(input) >= 3)
assert(isscalar(min_peak_height))
assert(isscalar(min_separation))

%input must be a column
input = input(:);

%if the aren't any values over the min_height, return zero
if max(input(2:(end-1))) < min_peak_height
    peaks = [];
else
    warning('off')
    [ratio_pk_amp, ratio_pk_loc, ratio_pk_width] = findpeaks(input, 'MinPeakHeight', min_peak_height, 'MinPeakDistance', min_separation); 
    warning('on')
    peaks = [ratio_pk_loc, ratio_pk_amp, ratio_pk_width];
end

end
