clear;

s = kSerial(115200, 'clear');
s.setRecordBufferSize(1024 * 16);
s.setRecvThreshold(0);
s.open();

t_idx = 1: 2;
for i = 1 : 100000
    [packetData, packetInfo, packetLens] = s.packetRecv();
    if ~isempty(packetLens) && packetLens > 0

        tims = s.ks.data(t_idx, :);
        tt   = s.getTime(t_idx, 0, 0.001);
        freq = s.getFreq(t_idx, 100, 0.001);

        fprintf('[%06i][%02i]', s.ks.lens, packetLens);
        fprintf('[%02i:%02i:%02i][%4.0fHz] ', tt(1), tt(2), fix(tt(3) / 10), freq);
if size(packetData, 1) == 3
        dist = s.ks.data(3, :) / 1000; % mm
        fprintf('dist[%6.3f]', dist(:, end));
elseif size(packetData, 1) == 5
        dist = s.ks.data(3: 5, :) / 1000; % mm
        fprintf('dist[%6.3f, %6.3f, %6.3f]', dist(:, end));
elseif size(packetData, 1) == 6
        dist = s.ks.data(3: 6, :) / 1000; % mm
        fprintf('dist[%6.3f, %6.3f, %6.3f, %6.3f]', dist(:, end));
end
        fprintf('\n');
    end
end
s.close();

% {
% check packet
[rate, lost, dt] = s.getLostRate(t_idx, freq, 0.001);
if lost == 0
    fprintf('---- [%05.2f%%] No packet loss ( %i / %i ) ----\n', rate * 100, lost, s.ks.lens);
else
    fprintf('---- [%05.2f%%] Packet loss ( %i / %i ) ----\n', rate * 100, lost, s.ks.lens);
end
% plot(1 : s.ks.lens - 1, dt)
%}
% {
s.save2mat('log/rawData', {'tims(1:2)'});
%}
