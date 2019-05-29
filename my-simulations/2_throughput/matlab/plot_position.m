A = dlmread('../input/location_400_0.txt');

for i = 25:25
    for j = 1:10
    B = dlmread(['../input/location_400_0_' int2str(i) '_' int2str(j) '.txt']);
    figure; hold on;
    scatter(A(:,1), A(:,2), 30, 'filled');
    scatter(B(:,1), B(:,2), 60, 'filled');
    scatter(B(1:3,1), B(1:3,2), 90, 'filled');
    box on
    grid on
    xlabel('x (meter)');
    ylabel('y (meter)');
    legend('client', 'router', 'gateway');
    set(gcf, 'Position', [500 500 800 800]);
    set(gca, 'Fontsize', 12);
    pause;
    close all;
    end
end