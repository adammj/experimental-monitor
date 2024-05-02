function delete_all_experimental_monitor_timers(obj) %#ok<INUSD>
%find all timers that were used by the experimental monitor


all_timers = timerfindall;
for i = length(all_timers):-1:1
    if strcmp(all_timers(i).UserData, 'experimental_monitor')
        try
            stop(all_timers(i));
            delete(all_timers(i));
        catch
            fprintf('error deleting timer\n');
        end
    end
end

end
