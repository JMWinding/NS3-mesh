xrange = 250;
yrange = 250;
wrange = 30;

nsamples = 400;
count = 0;

A = dlmread(['../input/location_' int2str(nsamples) '_' int2str(count) '.txt']);

for i = 3:6
    for j = i:6
        if (i*j < 10) || (i*j > 25)
            continue;
        end
        
        x2 = zeros(i*j,1);
        y2 = zeros(i*j,1);
        z2 = zeros(i*j,1);
        
        detx = xrange./(i+1);
        dety = yrange./(j+1);
        for k = 1:i*j
            x2(k) = detx * (0.5 + floor((k-1)/j));
            y2(k) = dety * (0.5 + mod(k-1,j));
        end
        
        z = zeros(size(A,1),1);
        dists = zeros(i*j,1);
        for kk = 1:size(A,1)
            for k = 1:i*j
                dists(k) = pdist([A(kk,1:2); x2(k) y2(k)]);
            end
            [mdist, z(kk)] = min(dists);
            if mdist < wrange
                z2(z(kk)) = z2(z(kk)) + 1;
            end
        end
        
        [z2, idx] = sort(z2, 'descend');
        x2 = x2(idx);
        y2 = y2(idx);

        filename = ['./grid_' int2str(nsamples) '_' int2str(count) ...
            '_' int2str(i*j) '.txt'];
        fid = fopen(filename, 'w');
        for k = 1:i*j
            fprintf(fid, '%.2f\t%.2f\t%.2f\t%f\n', x2(k), y2(k), 0.0, z2(k)*0.3);
        end    
        fclose(fid);
        
        disp([i*j sum(z2)]);
        k = k + 1;
    end
end