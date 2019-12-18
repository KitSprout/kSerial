
clc, clear, close all;
importSensorInfo

s = kSerial(115200, 'clear');
s.setRecvThreshold(0);
s.setRecordBufferSize(32*1024);
s.setRecordExpandSize(1024);
s.setCustomizeDataSize(0);
s.setPacketObserverWeighting(0.6);
s.setPacketObserverTimeunit(0.001);
s.setPacketObserverTimeindex(1:2);
s.open();

fig = figure(1);
set(fig, 'Position', [100, 200, 1200, 460], 'color', 'w');
ax = subplot(1, 1, 1);

% color, scale, offset, markersize
cvinfo = {
    'r', 1, 0, 12;
    'g', 1, 0, 12;
    'b', 1, 0, 12;
    'c', 1, 0, 12;
    'm', 1, 0, 12;
    'y', 1, 0, 12;
};
select = sv.g_idx;

osc = kOscilloscope(ax);
osc.setWindowSize(20000 * [1, -1], 800);
osc.setCurveBufferSize(16*1024);
osc.setCurveInfo(cvinfo);
osc.initOscilloscope(select, 'runtime', 'data');

while osc.available
    [packetInfo, packetData, packetLens] = s.packetRecv('record');
    if packetLens > 0
        [freq, tims] = s.packetObserver();

        fprintf('[%03d]', packetLens);
        fprintf('[%02i:%02i:%02i]', tims(2), tims(3), fix(tims(4) / 10));
        fprintf('[%4dHz] ', freq);
        for i = select
            fprintf('%5d ', packetData(i, end));
        end
        fprintf('\n');

        osc.updateOscilloscope(packetData);
    end
end
s.close();

% {
% check packet
[rate, lost, dc, type] = s.getPacketLostRate();
if lost == 0
    fprintf('\n---- [%05.2f%%] No packet loss ( %i / %i ) ----\n', rate * 100, lost, s.ks.lens);
else
    fprintf('\n---- [%05.2f%%] Packet loss ( %i / %i ) ----\n', rate * 100, lost, s.ks.lens);
end
% plot(1 : s.ks.lens - 1, dc)
%}
%{
filename = s.save2mat('log/rawdata', sv);
%}
