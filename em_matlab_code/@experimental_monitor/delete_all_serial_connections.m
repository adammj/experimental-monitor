function delete_all_serial_connections(obj) %#ok<INUSD>

%default way
try
    all_serial = instrfind;
    
    if ~isempty(all_serial)
        fprintf('delete all serial connections...\n');
    else
        return;
    end
    
    for i = 1:length(all_serial)
        current_serial_object = all_serial(i);
        fclose(current_serial_object);
        current_serial_object.BytesAvailableFcn = '';
        delete(current_serial_object);
    end
    
    if ~isempty(instrfind)
        fclose(instrfind);
        delete(instrfind);
    end
    
    if ~isempty(instrfind)
        fprintf('some serial objects remain\n');
    end
    return;
catch
end



%otherwise, do it manually
serial_port_cell_array = get_serial_ports();

if isempty(serial_port_cell_array)
    return;
end

fprintf('delete all serial connections...\n');

for i = 1:length(serial_port_cell_array)
    serial_port = serial_port_cell_array{i};
    fprintf('delete %s\n', serial_port);
    try
        s = serial(serial_port); %#ok<TNMLP>
    catch
    end

    try
        fopen(s);
    catch
    end

    try
        fclose(s);
    catch
    end

    try
        delete(s);
    catch
    end
end

end
