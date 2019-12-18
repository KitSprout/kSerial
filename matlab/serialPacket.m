
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

% {
while s.ks.lens < 2000
    [packetInfo, packetData, packetLens] = s.packetRecv('record');
    if packetLens > 0
        [freq, tims] = s.packetObserver();

%         s.updateCustomizeData(s.ks.lens .* ones(1, packetLens));

        fprintf('[%03d]', packetLens);
        fprintf('[%02i:%02i:%02i]', tims(2), tims(3), fix(tims(4) / 10));
        fprintf('[%4dHz] ', freq);
%         fprintf('%5d ', s.getParameter(s.ks.lens, 'uint16'));
        for i = 1 : packetInfo(6, end)
            fprintf('%5d ', packetData(i, end));
        end
        fprintf('\n');
    end
end
%}
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
