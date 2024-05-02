
fprintf("start\n");
clust = parcluster('local');


fprintf("periods\n");
%% timer parameter
period1=0.05;
period2=0.25;
abstime=1;

%% create tasks
fprintf("batch\n");
obj1= batch(clust,@timerfun_new,1,{period1,abstime});
fprintf("done1 %s\n",datestr(now,'HH:MM:SS.FFF'));
obj2= batch(clust,@timerfun_new,1,{period2,abstime});
fprintf("done2 %s\n", datestr(now,'HH:MM:SS.FFF'));

%% submit
fprintf("wait1 %s\n", datestr(now,'HH:MM:SS.FFF'));
wait(obj1);
fprintf("endwait1 %s\n", datestr(now,'HH:MM:SS.FFF'));
r1=fetchOutputs(obj1);
r1{1}
fprintf("wait 2\n");
wait(obj2);
fprintf("endwait2 %s\n", datestr(now,'HH:MM:SS.FFF'));
r2=fetchOutputs(obj2);
r2{1}



%% clean up
destroy(obj1);
destroy(obj2);