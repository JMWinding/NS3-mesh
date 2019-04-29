%% read .txt file
arange = [12 15 18 16 20 24 25];
brange = 1:1;
crange = 1:1:3;
drange = 10001:1:10005;
route = 'aodv-udp';

result2 = cell(length(arange), length(brange), length(crange));

for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            for dd = 1:length(drange) % random seed
                a = arange(aa);
                c = crange(cc);
                d = drange(dd);
                
                filename = ['../output/' route '/grid_400_0_' int2str(a) '_' ...
                    int2str(c) '_' int2str(d) '.txt'];
                fid = fopen(filename, 'r');
                for k = 1:61
                    l = fgetl(fid);
                end
                C = textscan(fid, '%s %f %f', 'Delimiter', '\t');
                fclose(fid);
                
                CC = [C{2} C{3}];
                result2{aa,bb,cc} = [result2{aa,bb,cc}; sum(C{2}), sum(C{2}.*C{3})./sum(C{2})];
            end
        end
    end
end

save(['grid_400_0_' route '2.mat'], 'result2');

%% throughput
A = zeros(length(arange), length(brange), length(crange));
B = zeros(length(arange), length(brange), length(crange));
for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            A(aa,bb,cc) = mean(result2{aa,bb,cc}(1))./1024;
            B(aa,bb,cc) = mean(result2{aa,bb,cc}(2))./1e6;
        end
    end
end

%%
scatter(arange.'+0.2, A(:,1,1), 100, 'x', 'MarkerEdgeColor', [0 0.4470 0.7410], 'LineWidth', 2);
scatter(arange.'+0.2, A(:,1,2), 100, 'x', 'MarkerEdgeColor', [0.8500 0.3250 0.0980], 'LineWidth', 2);
scatter(arange.'+0.2, A(:,1,3), 100, 'x', 'MarkerEdgeColor', [0.9290 0.6940 0.1250], 'LineWidth', 2);
legend('1 gateway', '2 gateways', '3 gateways');

scatter(arange.'+0.2, B(:,1,1), 100, 'x', 'MarkerEdgeColor', [0 0.4470 0.7410], 'LineWidth', 2);
scatter(arange.'+0.2, B(:,1,2), 100, 'x', 'MarkerEdgeColor', [0.8500 0.3250 0.0980], 'LineWidth', 2);
scatter(arange.'+0.2, B(:,1,3), 100, 'x', 'MarkerEdgeColor', [0.9290 0.6940 0.1250], 'LineWidth', 2);
legend('1 gateway', '2 gateways', '3 gateways');