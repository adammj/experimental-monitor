function [] = cleanup_all_em()
%remove all em and eyem instances
%including variables on the workspace

try
    eyem = eye_module.get_module;
    eyem.delete(true);
catch
end

try
    em = experimental_monitor.get_monitor;
    em.delete(true);
catch
end

%do an extra check on the base workspace
base_variables = evalin('base', 'whos');
for i = 1:length(base_variables)
    %test for eye_module
    if strcmp(base_variables(i).class, 'eye_module')
        try
            evalin('base', cat(2, 'clear ', base_variables(i).name));
        catch
        end
    end
    
    %test for experimental_monitor
    if strcmp(base_variables(i).class, 'experimental_monitor')
        try
            evalin('base', cat(2, 'clear ', base_variables(i).name));
        catch
        end
    end
end

end
