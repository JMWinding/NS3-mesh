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
temp = sortrows(result2{1}, 1);
temprange = 1:round(length(drange)*0.7);
temp = temp(temprange,:);
if ~exist('thr', 'var')
    thr = [];
end
thr = [thr; temp(:,1)];
if ~exist('delay', 'var')
    delay = [];
end
delay = [delay; temp(:,2)];
pro1 = strings(size(temp,1),1);
pro1(:) = route;
if ~exist('pro', 'var')
    pro = [];
end
pro = [pro; pro1];

%% figure
figure;
boxplot(thr, pro);
ylabel('whole mesh throughput (Mbps)');
set(gca, 'FontSize', 12);
saveas(gca, 'throughput.fig');
saveas(gca, 'throughput.jpg');

figure;
boxplot(delay/1e6, pro);
ylabel('per packet delay (s)');
set(gca, 'FontSize', 12);
saveas(gca, 'delay.fig');
saveas(gca, 'delay.jpg');