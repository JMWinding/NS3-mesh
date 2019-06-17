%% read .txt file
arange = 10:1:25;
brange = 1:1:10;
crange = 1:1:3;
drange = 10001:1:10016;
route = 'aodv-udp-ideal';

result2 = cell(length(arange), length(brange), length(crange));

for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            for dd = 1:length(drange) % random seed
                a = arange(aa);
                b = brange(bb);
                c = crange(cc);
                d = drange(dd);
                
                filename = ['../output/' route '/mesh_400_0_' int2str(a) '_' int2str(b) '_' ...
                    int2str(c) '_' int2str(d) '.txt'];
                fid = fopen(filename, 'r');
                for k = 1:138
                    l = fgetl(fid);
                end
                C = textscan(fid, '%s %f %f', 'Delimiter', '\t');
                fclose(fid);
                
                temp = C{2}<=50;
                C{2} = C{2}.*temp;
                
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
            A(aa,bb,cc) = mean(result2{aa,bb,cc}(:,1));
            B(aa,bb,cc) = mean(result2{aa,bb,cc}(:,2))./1e6;
        end
    end
end
temprange = 4:length(brange);

% 1
figure; hold on;
for cc = 1:length(crange)
    temp = sort(A(:,:,cc).', 1, 'descend');
    temp = temp(temprange,:);
    mmedian = median(temp);
    mmax = temp(1,:);
    mmin = temp(end,:);
    errorbar(arange, mmedian, mmin-mmedian, mmax-mmedian, 'LineWidth', 2);
end

xlabel('number of routers');
ylabel('whole mesh throughput (Mbps)');
legend('1 gateway', '2 gateways', '3 gateways');
set(gcf, 'Position', [400 400 900 600]);
set(gca, 'FontSize', 12);
title(route);

% 2
figure; hold on;
for cc = 1:length(crange)
    temp = sort(A(:,:,cc).', 1, 'descend');
    temp = temp(temprange,:);
    mmedian = median(temp)./arange;
    mmax = temp(1,:)./arange;
    mmin = temp(end,:)./arange;
    errorbar(arange, mmedian, mmin-mmedian, mmax-mmedian, 'LineWidth', 2);
end

xlabel('number of routers');
ylabel('per AP throughput (Mbps)');
legend('1 gateway', '2 gateways', '3 gateways');
set(gcf, 'Position', [400 400 900 600]);
set(gca, 'FontSize', 12);
title(route);

% 3
figure; hold on;
for cc = 1:length(crange)
    temp = sort(B(:,:,cc).', 1, 'descend');
    temp = temp(temprange,:);
    mmedian = median(temp);
    mmax = temp(1,:);
    mmin = temp(end,:);
    errorbar(arange, mmedian, mmin-mmedian, mmax-mmedian, 'LineWidth', 2);
end

xlabel('number of routers');
ylabel('per packet delay (s)');
legend('1 gateway', '2 gateways', '3 gateways');
set(gcf, 'Position', [400 400 900 600]);
set(gca, 'FontSize', 12);
title(route);