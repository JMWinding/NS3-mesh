arange = 180:10:220;
brange = 1:5;
crange = 1:24;

result = cell(length(arange), length(brange));

for aa = 1:length(arange)
    for bb = 1:length(brange)
        for cc = 1:length(crange)
            a = arange(aa);
            b = brange(bb);
            c = crange(cc);

            filename = ['aodv.result.' int2str(a) '.' int2str(b) '.' ...
                int2str(c)];
            C = dlmread(filename, '', 23, 0);
            CC = mean(C(:,2:end));
            result{aa,bb} = [result{aa,bb}; CC];

%                 figure; hold on;
%                 for i = 2:size(C,2)
%                     plot(C(1:end-1,1), diff(C(:,i))*8/1024/1024/(C(2,1)-C(1,1)));
%                 end
%                 close all;
        end
    end
end

% save('aodv-result.mat', 'result');

%%
for aa = 1:length(arange)
    for bb = 1:length(brange)
        figure;
        boxplot(result{aa,bb});
        xlabel('each router');
        ylabel('throughput (Mbps)');
        title(['scale = ' num2str(arange(aa)) ...
            ', expected throughput = ' num2str(brange(bb))]);
%             saveas(gcf, ['aodv-result-' num2str(arange(aa)) ...
%                 '-' num2str(brange(bb)) '.fig']);
        close all;
    end
end