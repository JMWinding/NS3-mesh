%% read .txt file
arange = 26:1:26;
brange = 1:1;
crange = 3:1:3;
drange = 10001:1:10100;
route = 'aodv_udp';

result1 = cell(length(arange), length(crange));
result2 = cell(length(arange), length(crange));

for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            for dd = 1:length(drange) % random seed
                a = arange(aa);
                c = crange(cc);
                d = drange(dd);
                
                filename = ['../output/mesh_' int2str(a) '_' ...
                    int2str(c) '_' int2str(d) '_' route '.txt'];
                fid = fopen(filename, 'r');
                for k = 1:138
                    l = fgetl(fid);
                end
                C = textscan(fid, '%s %f %f', 'Delimiter', '\t');
                fclose(fid);
                
                CC = [C{2} C{3}];
                result1{aa,bb,cc} = [result1{aa,bb,cc}, CC];
                result2{aa,bb,cc} = [result2{aa,bb,cc}; sum(C{2}), sum(C{2}.*C{3})./sum(C{2})];
            end
        end
    end
end

save(['mesh_400_0_' route '2.mat'], 'result1', 'result2');

%%
if ~exist('thr', 'var')
    thr = [];
end
thr = [thr; result2{1}(:,1)];
if ~exist('delay', 'var')
    delay = [];
end
delay = [delay; result2{1}(:,2)];
str = strings(length(drange),1);
str(:) = route;
if ~exist('pro', 'var')
    pro = [];
end
pro = [pro; str];

%% figure
figure;
boxplot(thr/1024, pro);
ylabel('whole mesh throughput (Mbps)');
set(gca, 'FontSize', 12);

figure;
boxplot(delay/1e6, pro);
ylabel('per packet delay (s)');
set(gca, 'FontSize', 12);