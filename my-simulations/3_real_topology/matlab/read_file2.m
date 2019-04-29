%% read .txt file
arange = 27:1:27;
brange = 1:1;
crange = 3:1:3;
drange = 10001:1:10022;
route = 'aodv_tcp';

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

save(['mesh_400_0_' route '2.mat'], 'result2');

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

figure; hold on;
for cc = 1:length(crange)
    mmean = mean(A(:,:,cc).');
    mmax = max(A(:,:,cc).');
    mmin = min(A(:,:,cc).');
    errorbar(arange, mmean, mmin-mmean, mmax-mmean, 'LineWidth', 2);
end

xlabel('number of routers');
ylabel('aggregated throughput (Mbps)');
legend('1 gateway', '2 gateways', '3 gateways');
set(gcf, 'Position', [400 400 900 600]);
set(gca, 'FontSize', 12);
title(route);

figure; hold on;
for cc = 1:length(crange)
    mmean = mean(B(:,:,cc).');
    mmax = max(B(:,:,cc).');
    mmin = min(B(:,:,cc).');
    errorbar(arange, mmean, mmin-mmean, mmax-mmean, 'LineWidth', 2);
end

xlabel('number of routers');
ylabel('average delay (s)');
legend('1 gateway', '2 gateways', '3 gateways');
set(gcf, 'Position', [400 400 900 600]);
set(gca, 'FontSize', 12);
title(route);