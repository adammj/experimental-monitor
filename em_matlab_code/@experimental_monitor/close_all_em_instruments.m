function close_all_em_instruments(obj) %#ok<INUSD>

all_instruments = instrfindall;
for i = 1:length(all_instruments)
    instrument = all_instruments(i);
    if strcmp(instrument.UserData, 'experimental_monitor')
        try
            fclose(instrument);
            delete(instrument);
        catch
        end
    end
end

end
