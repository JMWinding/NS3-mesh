for i = 25:-1:16
for j = 1:10
A = dlmread('../input/location_400_0.txt');
B = dlmread(['../input/location_400_0_' int2str(i) '_' int2str(j) '.txt']);
figure; hold on;
scatter(A(:,1), A(:,2), '.');
scatter(B(:,1), B(:,2), 'filled');
scatter(B(1:3,1), B(1:3,2), 'o', 'LineWidth', 2);
box on
grid on
xlabel('x (meter)');
ylabel('y (meter)');
legend('client', 'router', 'gateway');
set(gcf, 'Position', [500 500 600 600]);
pause;
close all;
end
end