% read .txt file
arange = 11:2:19;
brange = 1:1:10;
crange = 1:1:3;
drange = 10001:1:10006;

result = cell(length(arange), length(brange), length(crange));

for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            for dd = 1:length(drange) % random seed
                a = arange(aa);
                b = brange(bb);
                c = crange(cc);
                d = drange(dd);
                
                filename = ['../output/mesh_400_0_' int2str(a) '_' int2str(b) '_' ...
                    int2str(c) '_' int2str(d) '.txt'];
                C = dlmread(filename, '', 23, 0);
                CC = mean(C);
                result{aa,bb,cc} = [result{aa,bb,cc}; CC(1,2:end)];
            end
        end
    end
end

save('mesh_400_0_result.mat', 'result');

%%

A = zeros(length(arange), length(brange), length(crange));
for aa = 1:length(arange) % # stations
    for bb = 1:length(brange) % topology
        for cc = 1:length(crange) % # gateways
            A(aa,bb,cc) = sum(mean(result{aa,bb,cc}));
        end
    end
end