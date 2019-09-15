close all;

%% read .txt file
arange = 3:1:9;
brange = 1:1:sqrt(40);
crange = 1:1:1;
drange = 10001:1:10016;
route = 'olsr-udp-ideal-error';

result = cell(length(arange), length(brange), length(crange));

for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            for dd = 1:length(drange) % random seed
                a = arange(aa);
                b = brange(bb);
                c = crange(cc);
                d = drange(dd);
                
                if b > a
                    continue;
                end
                if a * b > 40
                    continue;
                end
                
                filename = ['../output/' route '/mesh_' int2str(a) '_' int2str(b) '_' ...
                    int2str(d) '.txt'];
                fid = fopen(filename, 'r');
                for k = 1:198
                    l = fgetl(fid);
                end
                C = textscan(fid, '%s %f %f', 'Delimiter', '\t');
                fclose(fid);
                
                temp = C{2}<=50;
                C{2} = C{2}.*temp;
                
                CC = [C{2} C{3}];
                result{aa,bb,cc} = [result{aa,bb,cc}; sum(C{2}), sum(C{2}.*C{3})./sum(C{2})];
            end
        end
    end
end

save(['mesh_' route '.mat'], 'result');

%% throughput
A = zeros(length(arange), length(brange), length(crange));
B = zeros(length(arange), length(brange), length(crange));
for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            if isempty(result{aa,bb,cc})
                continue;
            end
            A(aa,bb,cc) = mean(result{aa,bb,cc}(:,1));
            B(aa,bb,cc) = mean(result{aa,bb,cc}(:,2))./1e6;
        end
    end
end

% 1
figure(1);
bar3(A);
set(gca, 'YTickLabel', arange);
set(gca, 'XTickLabel', brange);
xlabel('Grid Size X');
ylabel('Grid Size Y');
zlabel('whole mesh throughput (Mbps)');
title(route);
saveas(gca, [route '_wholethr.fig']);
saveas(gca, [route '_wholethr.jpg']);

% 2
figure(2);
bar3(A./(arange.' * brange));
set(gca, 'YTickLabel', arange);
set(gca, 'XTickLabel', brange);
xlabel('Grid Size X');
ylabel('Grid Size Y');
zlabel('per AP throughput (Mbps)');
title(route);
saveas(gca, [route '_perthr.fig']);
saveas(gca, [route '_perthr.jpg']);

% 3
figure(3);
bar3(B);
set(gca, 'YTickLabel', arange);
set(gca, 'XTickLabel', brange);
xlabel('Grid Size X');
ylabel('Grid Size Y');
zlabel('per packet delay (s)');
title(route);
saveas(gca, [route '_delay.fig']);
saveas(gca, [route '_delay.jpg']);