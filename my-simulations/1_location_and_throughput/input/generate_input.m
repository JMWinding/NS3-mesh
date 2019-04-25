xrange = 400;
yrange = 400;

count = 0;

f1 = @(x)(x);
f2 = @(y)(y.^2);
nsamples = 400;
x = f1(rand(nsamples,1)) .* xrange;
y = f2(rand(nsamples,1)) .* yrange;

filename = ['location_' int2str(nsamples) '_' int2str(count) '.txt'];
fid = fopen(filename, 'w');
for j = 1:nsamples
    fprintf(fid, '%.2f\t%.2f\t%.2f\n', x(j), y(j), 0.0);
end
fclose(fid);

for nsamples2 = 10:25
    for k = 1:10
        x2 = rand(nsamples2,1) .* xrange;
        y2 = rand(nsamples2,1) .* yrange;
        z2 = zeros(size(x2));

        for kk = 1:5
            z = ones(size(x));
            for i = 1:nsamples
                dist = inf;
                for j = 1:nsamples2
                    dist2 = pdist([x(i) y(i); x2(j) y2(j)]);
                    if dist2 < dist
                        z(i) = j;
                        dist = dist2;
                    end
                end
            end

            for j = 1:nsamples2
                z2(j) = sum(z==j);
                x2(j) = mean(x(z==j));
                y2(j) = mean(y(z==j));
            end

            [z2, idx] = sort(z2, 'descend');
            x2 = x2(idx);
            y2 = y2(idx);
        end
    
%         figure; hold on;
%         scatter(x, y, '.');
%         scatter(x2, y2, 'filled', 'LineWidth', 5);
%         pause;
%         close all;

        if z2(end) == 0
            k = k - 1;
            continue;
        end

        filename = ['location_' int2str(nsamples) '_' int2str(count) ...
            '_' int2str(nsamples2) '_' int2str(k) '.txt'];
        fid = fopen(filename, 'w');
        for j = 1:nsamples2
            fprintf(fid, '%.2f\t%.2f\t%.2f\t%d\n', x2(j), y2(j), 0.0, sum(z==j));
        end    
        fclose(fid);
    end
end