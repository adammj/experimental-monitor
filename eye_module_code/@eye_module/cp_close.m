function cp_close(obj, varargin)

% try
%     if ~ishandle(obj.control_panel_h)
%         fprintf('handle is not valid, returning\n');
%         return;
%     end
% catch
% end

%first, disable streaming
try
    obj.stream_eye(false);
catch
end

normally_closed = false;

%close figure, without recusion
try
    if ishandle(obj.control_panel_h)
        obj.control_panel_ready = false;
        delete(obj.control_panel_h);

        normally_closed = true;
        %fprintf('closed normally\n');
    end
catch
end

%backup method to delete a broken eyem control panel
try
    if ~normally_closed
        delete(varargin{1});
        fprintf('force deleted handle\n');
    end
catch
end

end
