function [output_cell, output_string, text_container] = command_window_text(varargin)
%[handle] = findjobj(0, 'nomenu', 'class','XCmdWndView');

[cmdWin]=com.mathworks.mde.cmdwin.CmdWin.getInstance;
cmdWin_comps=get(cmdWin,'Components');
subcomps=get(cmdWin_comps(1),'Components');
text_container=get(subcomps(1),'Components');
output_string=get(text_container(1),'text');

output_cell=textscan(output_string,'%s','Delimiter','\r\n','MultipleDelimsAsOne',1);
output_cell=output_cell{1};

if nargin==1 && varargin{1}~=0 || nargin==0
    typed_commands=strncmp('>> ',output_cell,3);
    output_cell(typed_commands)=cellfun(@(t) t(4:end),output_cell(typed_commands),'UniformOutput',0);
end

end